#include "PointLightShadowPass.hpp"

#include "Assets/Scene.hpp"
#include "Assets/Vertex.hpp"
#include "Assets/Model.hpp"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/GameObject.h"
#include "Utilities/FileUtils.h"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/DescriptorBinding.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/DescriptorSetManager.hpp"
#include "Vulkan/DescriptorSets.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/DeviceMemory.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/ShaderModule.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace Vulkan::Game
{

// ─────────────────────────────────────────────────────────────────────────────
// Internal push-constant layout for the cubemap point light shadow vertex shader
// ─────────────────────────────────────────────────────────────────────────────

struct PointShadowPushConstant
{
	glm::mat4 WorldMatrix;     // 64 bytes
	uint32_t  LightIndex;      //  4 bytes
	uint32_t  CubemapFaceIdx;  //  4 bytes  →  total 72 bytes (≤ 128-byte minimum)
};
static_assert(sizeof(PointShadowPushConstant) <= 128,
			  "PointShadowPushConstant exceeds the guaranteed Vulkan minimum of 128 bytes");

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

PointLightShadowPass::PointLightShadowPass(const Vulkan::Device& device,
										   const uint32_t        imageCount,
										   PointShadowSettings   settings)
	: device_(device)
	, settings_(std::move(settings))
{
	CreateDepthResources();
	CreateRenderPass();
	CreateFramebuffers();
	CreateDescriptorSets(imageCount);
	CreatePipeline();
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — ResolveLightSettings
// ─────────────────────────────────────────────────────────────────────────────

PointLightShadowPass::ResolvedLightSettings
PointLightShadowPass::ResolveLightSettings(const uint32_t lightIndex) const
{
	const PointShadowLightSettings* ovr =
		(lightIndex < settings_.LightOverrides.size())
		? &settings_.LightOverrides[lightIndex]
		: nullptr;

	ResolvedLightSettings rs{};
	rs.Resolution              = (ovr && ovr->Resolution)              ? *ovr->Resolution              : settings_.Resolution;
	rs.DepthBiasEnable         = (ovr && ovr->DepthBiasEnable)         ? *ovr->DepthBiasEnable         : settings_.DepthBiasEnable;
	rs.DepthBiasConstantFactor = (ovr && ovr->DepthBiasConstantFactor) ? *ovr->DepthBiasConstantFactor : settings_.DepthBiasConstantFactor;
	rs.DepthBiasSlopeFactor    = (ovr && ovr->DepthBiasSlopeFactor)    ? *ovr->DepthBiasSlopeFactor    : settings_.DepthBiasSlopeFactor;
	rs.DepthBiasClamp          = (ovr && ovr->DepthBiasClamp)          ? *ovr->DepthBiasClamp          : settings_.DepthBiasClamp;
	rs.NearPlane               = (ovr && ovr->NearPlane)               ? *ovr->NearPlane               : settings_.NearPlane;
	rs.FarPlane                = (ovr && ovr->FarPlane)                ? *ovr->FarPlane                : settings_.FarPlane;
	return rs;
}

PointLightShadowPass::~PointLightShadowPass()
{
	// Reverse construction order.
	if (pipeline_ != VK_NULL_HANDLE)
		vkDestroyPipeline(device_.Handle(), pipeline_, nullptr);

	if (pipelineLayout_ != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(device_.Handle(), pipelineLayout_, nullptr);

	descriptorSetManager_.reset();
	pointLightVPBuffers_.clear();
	pointLightVPMemories_.clear();

	// Destroy per-light framebuffers and face views before the render pass they reference.
	for (auto& layer : pointLayers_)
	{
		// Destroy framebuffers first
		for (VkFramebuffer fb : layer.Framebuffers)
		{
			if (fb != VK_NULL_HANDLE)
				vkDestroyFramebuffer(device_.Handle(), fb, nullptr);
		}

		// Then destroy face views
		for (VkImageView faceView : layer.FaceViews)
		{
			if (faceView != VK_NULL_HANDLE)
				vkDestroyImageView(device_.Handle(), faceView, nullptr);
		}
	}
	pointLayers_.clear();

	if (renderPass_ != VK_NULL_HANDLE)
		vkDestroyRenderPass(device_.Handle(), renderPass_, nullptr);

	pointShadowSampler_.reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public — UpdateLightVP
// ─────────────────────────────────────────────────────────────────────────────

void PointLightShadowPass::UpdateLightVP(const uint32_t imageIndex, const Assets::Scene& scene)
{
	PointShadowUBO ubo{};

	for (const auto& light : scene.Lights())
	{
		if (light.LightType != Assets::LightProperties::Enum::PointLight)
			continue;

		const ResolvedLightSettings rs = ResolveLightSettings(ubo.Count);
		const glm::vec3 lightPos = glm::vec3(light.LightPos);

		// Store light position (used for depth calc in fragment shader)
		ubo.LightPositions[ubo.Count] = glm::vec4(lightPos, 0.0f);

		// Compute 6 perspective VP matrices (one per cubemap face)
		std::array<glm::mat4, 6> cubemapVP;
		ComputeCubemapVP(lightPos, rs, cubemapVP);

		// Store all 6 in the UBO
		for (uint32_t face = 0; face < 6; ++face)
		{
			ubo.CubemapViewProj[ubo.Count * 6 + face] = cubemapVP[face];
		}

		++ubo.Count;

		if (ubo.Count >= kMaxPointShadowLights)
			break;
	}

	activePointLightCount_ = ubo.Count;

	// Store the far plane so shadow/main shaders can use linear depth.
	ubo.FarPlane = settings_.FarPlane;

	void* data = pointLightVPMemories_[imageIndex].Map(0, sizeof(PointShadowUBO));
	std::memcpy(data, &ubo, sizeof(PointShadowUBO));
	pointLightVPMemories_[imageIndex].Unmap();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public — Render
// ─────────────────────────────────────────────────────────────────────────────

void PointLightShadowPass::Render(
	VkCommandBuffer      commandBuffer,
	const uint32_t       imageIndex,
	const Assets::Scene& scene)
{
	const VkClearValue clearDepth{ .depthStencil = { 1.0f, 0 } };

	VkRenderPassBeginInfo rpInfo{};
	rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.renderPass      = renderPass_;
	rpInfo.clearValueCount = 1;
	rpInfo.pClearValues    = &clearDepth;

	VkDescriptorSet ds = descriptorSetManager_->DescriptorSets().Handle(imageIndex);

	VkBuffer     vbuf[]    = { scene.VertexBuffer().Handle() };
	VkDeviceSize offsets[] = { 0 };

	// ── 6 sub-passes per active point light (one per cubemap face) ──────────
	for (uint32_t li = 0; li < kMaxPointShadowLights; ++li)
	{
		auto& layer = pointLayers_[li];

		if (li < activePointLightCount_)
		{
			const ResolvedLightSettings rs = ResolveLightSettings(li);

			// Render each of the 6 cubemap faces
			for (uint32_t face = 0; face < 6; ++face)
			{
				// ── Transition → DEPTH_STENCIL_ATTACHMENT_OPTIMAL ────────────────────
				TransitionCubemapImage(commandBuffer, layer,
									   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

				rpInfo.framebuffer      = layer.Framebuffers[face];
				rpInfo.renderArea.offset = { 0, 0 };
				rpInfo.renderArea.extent = { layer.Resolution, layer.Resolution };

				vkCmdBeginRenderPass(commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
				{
					VkViewport vp{};
					vp.x        = 0.0f;
					vp.y        = 0.0f;
					vp.width    = static_cast<float>(layer.Resolution);
					vp.height   = static_cast<float>(layer.Resolution);
					vp.minDepth = 0.0f;
					vp.maxDepth = 1.0f;

					VkRect2D sc{};
					sc.offset = { 0, 0 };
					sc.extent = { layer.Resolution, layer.Resolution };

					vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
					vkCmdSetViewport(commandBuffer, 0, 1, &vp);
					vkCmdSetScissor(commandBuffer, 0, 1, &sc);

					// Apply per-light dynamic depth bias
					if (rs.DepthBiasEnable)
						vkCmdSetDepthBias(commandBuffer,
										  rs.DepthBiasConstantFactor,
										  rs.DepthBiasClamp,
										  rs.DepthBiasSlopeFactor);
					else
						vkCmdSetDepthBias(commandBuffer, 0.0f, 0.0f, 0.0f);

					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
											pipelineLayout_, 0, 1, &ds, 0, nullptr);
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, vbuf, offsets);
					vkCmdBindIndexBuffer(commandBuffer, scene.IndexBuffer().Handle(), 0,
										 VK_INDEX_TYPE_UINT32);

					uint32_t vertexOffset = 0;
					uint32_t indexOffset  = 0;

					for (GameObject* go : ModelManager::getInstance()->getObjectList())
					{
						if (!go || !go->getModel()) continue;

						// Skip purely emissive meshes
						const auto& mats = go->getModel()->Materials();
						const bool allEmissive = !mats.empty() && std::all_of(
							mats.begin(), mats.end(),
							[](const Assets::Material& m)
							{ return m.MaterialModel == Assets::Material::Enum::DiffuseLight; });

						const uint32_t idxCount = static_cast<uint32_t>(go->getModel()->NumberOfIndices());
						const uint32_t vtxCount = static_cast<uint32_t>(go->getModel()->NumberOfVertices());

						if (!allEmissive)
						{
							PointShadowPushConstant pc{};
							pc.WorldMatrix      = go->getWorldMatrix();
							pc.LightIndex       = li;
							pc.CubemapFaceIdx   = face;

							vkCmdPushConstants(commandBuffer, pipelineLayout_,
											   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
											   sizeof(PointShadowPushConstant), &pc);

							vkCmdDrawIndexed(commandBuffer, idxCount, 1,
											 indexOffset, static_cast<int32_t>(vertexOffset), 0);
						}

						vertexOffset += vtxCount;
						indexOffset  += idxCount;
					}
				}
				vkCmdEndRenderPass(commandBuffer);

				// ── Transition → SHADER_READ_ONLY_OPTIMAL after this face ──────────
				// Note: we do this after each face to ensure proper synchronization
			}

			// Transition to SHADER_READ_ONLY_OPTIMAL once all 6 faces are done
			TransitionCubemapImage(commandBuffer, layer,
							   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		else
		{
			// Inactive light — ensure the image is in SHADER_READ_ONLY_OPTIMAL
			// so the main pass can safely sample it (even if unused).
			TransitionCubemapImage(commandBuffer, layer,
							   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Public — Accessors
// ─────────────────────────────────────────────────────────────────────────────

std::vector<VkImageView> PointLightShadowPass::PointShadowImageViews() const
{
	std::vector<VkImageView> views;
	views.reserve(pointLayers_.size());
	for (const auto& layer : pointLayers_)
		views.push_back(layer.CubemapView->Handle());
	return views;
}

VkSampler PointLightShadowPass::PointShadowSampler() const
{
	return pointShadowSampler_->Handle();
}

const Vulkan::Buffer& PointLightShadowPass::PointLightVPBuffer(const uint32_t imageIndex) const
{
	return *pointLightVPBuffers_[imageIndex];
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateDepthResources
// ─────────────────────────────────────────────────────────────────────────────

void PointLightShadowPass::CreateDepthResources()
{
	const VkFormat fmt = settings_.DepthFormat;

	pointLayers_.resize(kMaxPointShadowLights);

	for (uint32_t i = 0; i < kMaxPointShadowLights; ++i)
	{
		const ResolvedLightSettings rs = ResolveLightSettings(i);
		auto& layer = pointLayers_[i];
		layer.Resolution = rs.Resolution;

		const VkExtent2D extent = { rs.Resolution, rs.Resolution };

		// Create a 2D array image with 6 layers for cubemap faces
		layer.CubemapImage = std::make_unique<Vulkan::Image>(
			device_, extent, fmt,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			6,                                           // 6 array layers (cubemap faces)
			VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);       // cube-compatible flag

		layer.Memory = std::make_unique<Vulkan::DeviceMemory>(
			layer.CubemapImage->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

		// Create a view of the entire cubemap (all 6 layers as a cube)
		layer.CubemapView = std::make_unique<Vulkan::ImageView>(
			device_, 
			layer.CubemapImage->Handle(), 
			fmt, 
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_VIEW_TYPE_CUBE,   // view as cubemap
			6);                         // all 6 layers
	}

	// ── Shared cubemap compare sampler ──────────────────────────────────────
	Vulkan::SamplerConfig cfg{};
	cfg.MagFilter        = settings_.MagFilter;
	cfg.MinFilter        = settings_.MinFilter;
	cfg.AddressModeU     = settings_.AddressMode;
	cfg.AddressModeV     = settings_.AddressMode;
	cfg.AddressModeW     = settings_.AddressMode;
	cfg.BorderColor      = settings_.BorderColor;
	cfg.AnisotropyEnable = false;
	cfg.CompareEnable    = true;
	cfg.CompareOp        = settings_.CompareOp;
	cfg.MipmapMode       = settings_.MipmapMode;

	pointShadowSampler_ = std::make_unique<Vulkan::Sampler>(device_, cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateRenderPass
// ─────────────────────────────────────────────────────────────────────────────

void PointLightShadowPass::CreateRenderPass()
{
	// Single depth attachment, no colour (same as directional)
	VkAttachmentDescription depthAtt{};
	depthAtt.format         = settings_.DepthFormat;
	depthAtt.samples        = VK_SAMPLE_COUNT_1_BIT;
	depthAtt.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAtt.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	depthAtt.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAtt.initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAtt.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthRef{};
	depthRef.attachment = 0;
	depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount    = 0;
	subpass.pDepthStencilAttachment = &depthRef;

	std::array<VkSubpassDependency, 2> deps{};

	deps[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
	deps[0].dstSubpass      = 0;
	deps[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	deps[0].dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	deps[0].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
	deps[0].dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	deps[1].srcSubpass      = 0;
	deps[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
	deps[1].srcStageMask    = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	deps[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	deps[1].srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	deps[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
	deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo rpInfo{};
	rpInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rpInfo.attachmentCount = 1;
	rpInfo.pAttachments    = &depthAtt;
	rpInfo.subpassCount    = 1;
	rpInfo.pSubpasses      = &subpass;
	rpInfo.dependencyCount = static_cast<uint32_t>(deps.size());
	rpInfo.pDependencies   = deps.data();

	Check(vkCreateRenderPass(device_.Handle(), &rpInfo, nullptr, &renderPass_),
		  "create point light shadow map render pass");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateFramebuffers
// ─────────────────────────────────────────────────────────────────────────────

void PointLightShadowPass::CreateFramebuffers()
{
	for (auto& layer : pointLayers_)
	{
		for (uint32_t face = 0; face < 6; ++face)
		{
			// Create a view for a single cubemap face (2D array layer)
			VkImageViewCreateInfo faceViewInfo{};
			faceViewInfo.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			faceViewInfo.image      = layer.CubemapImage->Handle();
			faceViewInfo.viewType   = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			faceViewInfo.format     = settings_.DepthFormat;
			faceViewInfo.components = {
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
			};
			faceViewInfo.subresourceRange = {
				VK_IMAGE_ASPECT_DEPTH_BIT,
				0,                          // baseMipLevel
				1,                          // levelCount
				face,                       // baseArrayLayer (single face)
				1                           // layerCount
			};

			// Create and KEEP the face view (framebuffer stores only the handle)
			Check(vkCreateImageView(device_.Handle(), &faceViewInfo, nullptr, &layer.FaceViews[face]),
				  "create point light shadow cubemap face view");

			// Create framebuffer for this face
			const VkImageView attachments[] = { layer.FaceViews[face] };

			VkFramebufferCreateInfo fbInfo{};
			fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbInfo.renderPass      = renderPass_;
			fbInfo.attachmentCount = 1;
			fbInfo.pAttachments    = attachments;
			fbInfo.width           = layer.Resolution;
			fbInfo.height          = layer.Resolution;
			fbInfo.layers          = 1;

			Check(vkCreateFramebuffer(device_.Handle(), &fbInfo, nullptr, &layer.Framebuffers[face]),
				  "create point light shadow map framebuffer");
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateDescriptorSets
// ─────────────────────────────────────────────────────────────────────────────

void PointLightShadowPass::CreateDescriptorSets(const uint32_t imageCount)
{
	const std::vector<DescriptorBinding> bindings =
	{
		{ 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
	};

	descriptorSetManager_ = std::make_unique<DescriptorSetManager>(
		device_, bindings, imageCount);

	auto& sets = descriptorSetManager_->DescriptorSets();

	pointLightVPBuffers_.reserve(imageCount);
	pointLightVPMemories_.reserve(imageCount);

	for (uint32_t i = 0; i < imageCount; ++i)
	{
		pointLightVPBuffers_.push_back(std::make_unique<Vulkan::Buffer>(
			device_, sizeof(PointShadowUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));

		pointLightVPMemories_.push_back(
			pointLightVPBuffers_.back()->AllocateMemory(
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		const PointShadowUBO defaultUBO{};
		void* data = pointLightVPMemories_.back().Map(0, sizeof(PointShadowUBO));
		std::memcpy(data, &defaultUBO, sizeof(PointShadowUBO));
		pointLightVPMemories_.back().Unmap();

		VkDescriptorBufferInfo bufInfo{};
		bufInfo.buffer = pointLightVPBuffers_.back()->Handle();
		bufInfo.offset = 0;
		bufInfo.range  = sizeof(PointShadowUBO);

		const std::vector<VkWriteDescriptorSet> writes = { sets.Bind(i, 0, bufInfo) };
		sets.UpdateDescriptors(i, writes);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreatePipeline
// ─────────────────────────────────────────────────────────────────────────────

void PointLightShadowPass::CreatePipeline()
{
	const std::string shaderDir =
		FileUtils::getAssetsFolderPath().generic_string() + "/shaders/";
	const ShaderModule vertShader(device_, shaderDir + "point_shadow_vert.vert.spv");
	const ShaderModule fragShader(device_, shaderDir + "point_shadow_frag.frag.spv");

	const VkPipelineShaderStageCreateInfo vertStage =
		vertShader.CreateShaderStage(VK_SHADER_STAGE_VERTEX_BIT);
	const VkPipelineShaderStageCreateInfo fragStage =
		fragShader.CreateShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT);

	const auto bindingDesc    = Assets::Vertex::GetBindingDescription();
	const auto attributeDescs = Assets::Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertInput{};
	vertInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertInput.vertexBindingDescriptionCount   = 1;
	vertInput.pVertexBindingDescriptions      = &bindingDesc;
	vertInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
	vertInput.pVertexAttributeDescriptions    = attributeDescs.data();

	VkPipelineInputAssemblyStateCreateInfo ia{};
	ia.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	ia.primitiveRestartEnable = VK_FALSE;

	const std::array<VkDynamicState, 3> dynStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_DEPTH_BIAS
	};
	VkPipelineDynamicStateCreateInfo dynState{};
	dynState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynState.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
	dynState.pDynamicStates    = dynStates.data();

	VkPipelineViewportStateCreateInfo vpState{};
	vpState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vpState.viewportCount = 1;
	vpState.scissorCount  = 1;

	VkPipelineRasterizationStateCreateInfo rast{};
	rast.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rast.depthClampEnable        = VK_FALSE;
	rast.rasterizerDiscardEnable = VK_FALSE;
	rast.polygonMode             = VK_POLYGON_MODE_FILL;
	rast.lineWidth               = 1.0f;
	rast.cullMode                = settings_.CullMode;
	rast.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rast.depthBiasEnable         = VK_TRUE;
	rast.depthBiasConstantFactor = 0.0f;
	rast.depthBiasSlopeFactor    = 0.0f;
	rast.depthBiasClamp          = 0.0f;

	VkPipelineMultisampleStateCreateInfo ms{};
	ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.sampleShadingEnable  = VK_FALSE;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo ds{};
	ds.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.depthTestEnable  = VK_TRUE;
	ds.depthWriteEnable = VK_TRUE;
	ds.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineColorBlendStateCreateInfo blend{};
	blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.logicOpEnable   = VK_FALSE;
	blend.attachmentCount = 0;
	blend.pAttachments    = nullptr;

	const VkDescriptorSetLayout dsl =
		descriptorSetManager_->DescriptorSetLayout().Handle();

	VkPushConstantRange pcRange{};
	pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pcRange.offset     = 0;
	pcRange.size       = sizeof(PointShadowPushConstant);

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 1;
	layoutInfo.pSetLayouts            = &dsl;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &pcRange;

	Check(vkCreatePipelineLayout(device_.Handle(), &layoutInfo, nullptr, &pipelineLayout_),
		  "create point light shadow map pipeline layout");

	const VkPipelineShaderStageCreateInfo shaderStages[] = { vertStage, fragStage };

	VkGraphicsPipelineCreateInfo pipeInfo{};
	pipeInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeInfo.stageCount          = 2;
	pipeInfo.pStages             = shaderStages;
	pipeInfo.pVertexInputState   = &vertInput;
	pipeInfo.pInputAssemblyState = &ia;
	pipeInfo.pViewportState      = &vpState;
	pipeInfo.pRasterizationState = &rast;
	pipeInfo.pMultisampleState   = &ms;
	pipeInfo.pDepthStencilState  = &ds;
	pipeInfo.pColorBlendState    = &blend;
	pipeInfo.pDynamicState       = &dynState;
	pipeInfo.layout              = pipelineLayout_;
	pipeInfo.renderPass          = renderPass_;
	pipeInfo.subpass             = 0;

	Check(vkCreateGraphicsPipelines(device_.Handle(), VK_NULL_HANDLE,
									1, &pipeInfo, nullptr, &pipeline_),
		  "create point light shadow map graphics pipeline");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — ComputeCubemapVP
// ─────────────────────────────────────────────────────────────────────────────

void PointLightShadowPass::ComputeCubemapVP(
	const glm::vec3& lightPos,
	const ResolvedLightSettings& rs,
	std::array<glm::mat4, 6>& outVP) const
{
	// 6 cubemap face directions and their "up" vectors
	const std::array<glm::vec3, 6> directions = {{
		glm::vec3(1.0f,  0.0f,  0.0f),   // +X
		glm::vec3(-1.0f, 0.0f,  0.0f),   // -X
		glm::vec3(0.0f,  1.0f,  0.0f),   // +Y
		glm::vec3(0.0f, -1.0f,  0.0f),   // -Y
		glm::vec3(0.0f,  0.0f,  1.0f),   // +Z
		glm::vec3(0.0f,  0.0f, -1.0f)    // -Z
	}};

	const std::array<glm::vec3, 6> ups = {{
		glm::vec3(0.0f, -1.0f, 0.0f),    // +X: -Y up
		glm::vec3(0.0f, -1.0f, 0.0f),    // -X: -Y up
		glm::vec3(0.0f,  0.0f, 1.0f),    // +Y: +Z up
		glm::vec3(0.0f,  0.0f, -1.0f),   // -Y: -Z up
		glm::vec3(0.0f, -1.0f, 0.0f),    // +Z: -Y up
		glm::vec3(0.0f, -1.0f, 0.0f)     // -Z: -Y up
	}};

	// Perspective projection (90° FOV for cubemap)
	const glm::mat4 proj = glm::perspective(
		glm::radians(90.0f),  // 90-degree FOV for cubemap faces
		1.0f,                  // aspect ratio (square faces)
		rs.NearPlane,
		rs.FarPlane);

	// Flip Y for Vulkan (Y-down NDC)
	glm::mat4 projVulkan = proj;
	projVulkan[1][1] *= -1.0f;

	// Compute view-projection for each face
	for (uint32_t face = 0; face < 6; ++face)
	{
		const glm::vec3 target = lightPos + directions[face];
		const glm::mat4 view = glm::lookAt(lightPos, target, ups[face]);
		outVP[face] = projVulkan * view;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — TransitionCubemapImage
// ─────────────────────────────────────────────────────────────────────────────

void PointLightShadowPass::TransitionCubemapImage(
	VkCommandBuffer commandBuffer,
	PointShadowLayer& layer,
	VkImageLayout newLayout) const
{
	const VkImageLayout oldLayout = layer.CurrentLayout;
	if (oldLayout == newLayout)
		return;

	VkImageMemoryBarrier barrier{};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout           = oldLayout;
	barrier.newLayout           = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = layer.CubemapImage->Handle();
	barrier.subresourceRange    = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 6 };  // all 6 faces

	VkPipelineStageFlags srcStage{};
	VkPipelineStageFlags dstStage{};

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
							  | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
			 newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
							  | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
			 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
			 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		// Inactive-light cubemap: never written, just needs to be readable.
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::runtime_error(
			"PointLightShadowPass: unsupported cubemap image layout transition");
	}

	vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0,
						0, nullptr, 0, nullptr, 1, &barrier);

	layer.CurrentLayout = newLayout;
}

} // namespace Vulkan::Game
