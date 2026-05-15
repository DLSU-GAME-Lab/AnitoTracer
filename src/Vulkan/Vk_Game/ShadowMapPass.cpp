#include "ShadowMapPass.hpp"

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

// glm/gtc/matrix_transform.hpp is already pulled in via Utilities/Glm.hpp (included from ShadowMapPass.hpp)

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace Vulkan::Game
{

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

ShadowMapPass::ShadowMapPass(const Vulkan::Device& device, const uint32_t imageCount)
	: device_(device)
{
	CreateDepthResources();
	CreateRenderPass();
	CreateFramebuffer();
	CreateDescriptorSets(imageCount);
	CreatePipeline();
}

ShadowMapPass::~ShadowMapPass()
{
	// Reverse construction order — pipeline first, image last.
	if (pipeline_ != VK_NULL_HANDLE)
		vkDestroyPipeline(device_.Handle(), pipeline_, nullptr);

	if (pipelineLayout_ != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(device_.Handle(), pipelineLayout_, nullptr);

	// UBO descriptor sets managed by DescriptorSetManager
	descriptorSetManager_.reset();
	lightVPBuffers_.clear();
	lightVPMemories_.clear();

	if (framebuffer_ != VK_NULL_HANDLE)
		vkDestroyFramebuffer(device_.Handle(), framebuffer_, nullptr);

	if (renderPass_ != VK_NULL_HANDLE)
		vkDestroyRenderPass(device_.Handle(), renderPass_, nullptr);

	// RAII wrappers handle their own cleanup
	shadowSampler_.reset();
	shadowImageView_.reset();
	shadowMemory_.reset();
	shadowImage_.reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public — UpdateLightVP
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::UpdateLightVP(const uint32_t imageIndex, const Assets::Scene& scene)
{
	const ShadowUBO ubo = ComputeLightVP(scene);

	void* data = lightVPMemories_[imageIndex].Map(0, sizeof(ShadowUBO));
	std::memcpy(data, &ubo, sizeof(ShadowUBO));
	lightVPMemories_[imageIndex].Unmap();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public — Render
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::Render(
	VkCommandBuffer      commandBuffer,
	const uint32_t       imageIndex,
	const Assets::Scene& scene)
{
	// ── 1. Transition depth image → DEPTH_STENCIL_ATTACHMENT_OPTIMAL ─────────
	TransitionDepthImage(commandBuffer, currentLayout_,
						 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	currentLayout_ = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// ── 2. Begin depth-only render pass ───────────────────────────────────────
	VkClearValue clearDepth{};
	clearDepth.depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo rpInfo{};
	rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.renderPass        = renderPass_;
	rpInfo.framebuffer       = framebuffer_;
	rpInfo.renderArea.offset = { 0, 0 };
	rpInfo.renderArea.extent = { kSize, kSize };
	rpInfo.clearValueCount   = 1;
	rpInfo.pClearValues      = &clearDepth;

	vkCmdBeginRenderPass(commandBuffer, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

		// Set dynamic viewport + scissor for the shadow resolution
		VkViewport vp{};
		vp.x        = 0.0f;
		vp.y        = 0.0f;
		vp.width    = static_cast<float>(kSize);
		vp.height   = static_cast<float>(kSize);
		vp.minDepth = 0.0f;
		vp.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &vp);

		VkRect2D sc{};
		sc.offset = { 0, 0 };
		sc.extent = { kSize, kSize };
		vkCmdSetScissor(commandBuffer, 0, 1, &sc);

		// Bind shadow UBO descriptor (binding 0 = ShadowUBO)
		VkDescriptorSet ds = descriptorSetManager_->DescriptorSets().Handle(imageIndex);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
								pipelineLayout_, 0, 1, &ds, 0, nullptr);

		// Bind scene geometry
		VkBuffer     vbuf[]    = { scene.VertexBuffer().Handle() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vbuf, offsets);
		vkCmdBindIndexBuffer(commandBuffer, scene.IndexBuffer().Handle(), 0,
							 VK_INDEX_TYPE_UINT32);

		// Draw all GameObjects (same loop as the main forward pass).
		// Skip objects whose every material is DiffuseLight (emissive-only ray-tracer
		// area lights) — they cast no meaningful shadow and don't belong here.
		uint32_t vertexOffset = 0;
		uint32_t indexOffset  = 0;

		for (GameObject* go : ModelManager::getInstance()->getObjectList())
		{
			if (!go || !go->getModel()) continue;

			// Skip purely emissive meshes (ray-tracer area lights)
			{
				const auto& mats = go->getModel()->Materials();
				const bool allEmissive = !mats.empty() && std::all_of(
					mats.begin(), mats.end(),
					[](const Assets::Material& m){ return m.MaterialModel == Assets::Material::Enum::DiffuseLight; });
				if (allEmissive) { indexOffset += static_cast<uint32_t>(go->getModel()->NumberOfIndices()); vertexOffset += static_cast<uint32_t>(go->getModel()->NumberOfVertices()); continue; }
			}

			const glm::mat4 world = go->getWorldMatrix();
			vkCmdPushConstants(commandBuffer, pipelineLayout_,
							   VK_SHADER_STAGE_VERTEX_BIT, 0,
							   sizeof(glm::mat4), &world);

			const uint32_t idxCount = static_cast<uint32_t>(go->getModel()->NumberOfIndices());
			const uint32_t vtxCount = static_cast<uint32_t>(go->getModel()->NumberOfVertices());

			vkCmdDrawIndexed(commandBuffer, idxCount, 1,
							 indexOffset, static_cast<int32_t>(vertexOffset), 0);

			vertexOffset += vtxCount;
			indexOffset  += idxCount;
		}
	}
	vkCmdEndRenderPass(commandBuffer);

	// ── 3. Transition → SHADER_READ_ONLY_OPTIMAL for the main pass ────────────
	TransitionDepthImage(commandBuffer,
						 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
						 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	currentLayout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public — Accessors
// ─────────────────────────────────────────────────────────────────────────────

VkImageView ShadowMapPass::ShadowImageView() const
{
	return shadowImageView_->Handle();
}

VkSampler ShadowMapPass::ShadowSampler() const
{
	return shadowSampler_->Handle();
}

const Vulkan::Buffer& ShadowMapPass::LightVPBuffer(const uint32_t imageIndex) const
{
	return *lightVPBuffers_[imageIndex];
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateDepthResources
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::CreateDepthResources()
{
	constexpr VkFormat    kDepthFormat = VK_FORMAT_D32_SFLOAT;
	const     VkExtent2D  extent       = { kSize, kSize };

	// ── Depth image: can be sampled AND used as depth attachment ─────────────
	shadowImage_ = std::make_unique<Vulkan::Image>(
		device_, extent, kDepthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	shadowMemory_ = std::make_unique<Vulkan::DeviceMemory>(
		shadowImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

	// ── Depth image view ──────────────────────────────────────────────────────
	shadowImageView_ = std::make_unique<Vulkan::ImageView>(
		device_, shadowImage_->Handle(), kDepthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT);

	// ── Shadow sampler: compare-enabled for sampler2DShadow PCF reads ─────────
	// CLAMP_TO_BORDER + FLOAT_OPAQUE_WHITE ensures pixels outside the shadow
	// frustum are treated as fully lit (shadow value = 1.0 = not in shadow).
	Vulkan::SamplerConfig cfg{};
	cfg.MagFilter        = VK_FILTER_LINEAR;
	cfg.MinFilter        = VK_FILTER_LINEAR;
	cfg.AddressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	cfg.AddressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	cfg.AddressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	cfg.BorderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	cfg.AnisotropyEnable = false;
	cfg.CompareEnable    = true;
	cfg.CompareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
	cfg.MipmapMode       = VK_SAMPLER_MIPMAP_MODE_NEAREST;

	shadowSampler_ = std::make_unique<Vulkan::Sampler>(device_, cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateRenderPass
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::CreateRenderPass()
{
	// Single depth attachment — no colour attachments.
	// initialLayout = DEPTH_STENCIL_ATTACHMENT_OPTIMAL because our barrier
	// transitions the image before vkCmdBeginRenderPass is called.
	VkAttachmentDescription depthAtt{};
	depthAtt.format         = VK_FORMAT_D32_SFLOAT;
	depthAtt.samples        = VK_SAMPLE_COUNT_1_BIT;
	depthAtt.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAtt.storeOp        = VK_ATTACHMENT_STORE_OP_STORE; // keep depth for sampling
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

	// Subpass dependency 0: ensure the previous frame's main-pass fragment
	// reads complete before we start writing depth again.
	// Subpass dependency 1: ensure shadow depth writes complete before the
	// main pass fragment shader samples the shadow map.
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
		  "create shadow map render pass");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateFramebuffer
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::CreateFramebuffer()
{
	const VkImageView attachments[] = { shadowImageView_->Handle() };

	VkFramebufferCreateInfo fbInfo{};
	fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbInfo.renderPass      = renderPass_;
	fbInfo.attachmentCount = 1;
	fbInfo.pAttachments    = attachments;
	fbInfo.width           = kSize;
	fbInfo.height          = kSize;
	fbInfo.layers          = 1;

	Check(vkCreateFramebuffer(device_.Handle(), &fbInfo, nullptr, &framebuffer_),
		  "create shadow map framebuffer");
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreateDescriptorSets
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::CreateDescriptorSets(const uint32_t imageCount)
{
	// Binding 0: ShadowUBO (UNIFORM_BUFFER, vertex stage only)
	const std::vector<DescriptorBinding> bindings =
	{
		{ 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT }
	};

	descriptorSetManager_ = std::make_unique<DescriptorSetManager>(
		device_, bindings, imageCount);

	auto& sets = descriptorSetManager_->DescriptorSets();

	lightVPBuffers_.reserve(imageCount);
	lightVPMemories_.reserve(imageCount);

	for (uint32_t i = 0; i < imageCount; ++i)
	{
		// Host-visible UBO — updated every frame via memcpy
		lightVPBuffers_.push_back(std::make_unique<Vulkan::Buffer>(
			device_, sizeof(ShadowUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));

		lightVPMemories_.push_back(
			lightVPBuffers_.back()->AllocateMemory(
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

		// Initialise to identity so the first frame doesn't read garbage
		const ShadowUBO defaultUBO{};
		void* data = lightVPMemories_.back().Map(0, sizeof(ShadowUBO));
		std::memcpy(data, &defaultUBO, sizeof(ShadowUBO));
		lightVPMemories_.back().Unmap();

		// Write descriptor
		VkDescriptorBufferInfo bufInfo{};
		bufInfo.buffer = lightVPBuffers_.back()->Handle();
		bufInfo.offset = 0;
		bufInfo.range  = sizeof(ShadowUBO);

		const std::vector<VkWriteDescriptorSet> writes = { sets.Bind(i, 0, bufInfo) };
		sets.UpdateDescriptors(i, writes);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — CreatePipeline
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::CreatePipeline()
{
	const std::string shaderDir =
		FileUtils::getAssetsFolderPath().generic_string() + "/shaders/";
	const ShaderModule vertShader(device_, shaderDir + "shadow_vert.vert.spv");

	const VkPipelineShaderStageCreateInfo vertStage =
		vertShader.CreateShaderStage(VK_SHADER_STAGE_VERTEX_BIT);

	// ── Vertex input: full Assets::Vertex layout (position at location 0) ─────
	const auto bindingDesc    = Assets::Vertex::GetBindingDescription();
	const auto attributeDescs = Assets::Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertInput{};
	vertInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertInput.vertexBindingDescriptionCount   = 1;
	vertInput.pVertexBindingDescriptions      = &bindingDesc;
	vertInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
	vertInput.pVertexAttributeDescriptions    = attributeDescs.data();

	VkPipelineInputAssemblyStateCreateInfo ia{};
	ia.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	ia.primitiveRestartEnable = VK_FALSE;

	// ── Dynamic viewport + scissor (shadow resolution set at draw time) ────────
	const std::array<VkDynamicState, 2> dynStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynState{};
	dynState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynState.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
	dynState.pDynamicStates    = dynStates.data();

	VkPipelineViewportStateCreateInfo vpState{};
	vpState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vpState.viewportCount = 1;
	vpState.scissorCount  = 1;

	// ── Rasterizer: front-face cull (Peter-Panning fix) + depth bias ──────────
	VkPipelineRasterizationStateCreateInfo rast{};
	rast.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rast.depthClampEnable        = VK_FALSE;
	rast.rasterizerDiscardEnable = VK_FALSE;
	rast.polygonMode             = VK_POLYGON_MODE_FILL;
	rast.lineWidth               = 1.0f;
	rast.cullMode                = VK_CULL_MODE_BACK_BIT;  // standard back-face cull; depth bias handles acne
	rast.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rast.depthBiasEnable         = VK_TRUE;
	rast.depthBiasConstantFactor = 1.25f;  // constant offset (prevents surface acne)
	rast.depthBiasSlopeFactor    = 1.75f;  // slope-scaled offset (handles grazing angles)
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

	// ── No colour attachments (depth-only pass) ───────────────────────────────
	VkPipelineColorBlendStateCreateInfo blend{};
	blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.logicOpEnable   = VK_FALSE;
	blend.attachmentCount = 0;
	blend.pAttachments    = nullptr;

	// ── Pipeline layout: descriptor set 0 + push constant for model matrix ────
	const VkDescriptorSetLayout dsl =
		descriptorSetManager_->DescriptorSetLayout().Handle();

	VkPushConstantRange pcRange{};
	pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pcRange.offset     = 0;
	pcRange.size       = sizeof(glm::mat4);

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 1;
	layoutInfo.pSetLayouts            = &dsl;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &pcRange;

	Check(vkCreatePipelineLayout(device_.Handle(), &layoutInfo, nullptr, &pipelineLayout_),
		  "create shadow map pipeline layout");

	// ── Graphics pipeline (vertex stage only — no fragment shader needed) ─────
	VkGraphicsPipelineCreateInfo pipeInfo{};
	pipeInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeInfo.stageCount          = 1; // vertex only
	pipeInfo.pStages             = &vertStage;
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
		  "create shadow map graphics pipeline");
}

// ─────────────────────────────────────────────────────────────────────────────
// Static — ComputeLightVP
// ─────────────────────────────────────────────────────────────────────────────

ShadowMapPass::ShadowUBO ShadowMapPass::ComputeLightVP(const Assets::Scene& scene)
{
	// ── 1. Find the first directional light ───────────────────────────────────
	glm::vec3 lightDir(0.0f, -1.0f, 0.0f); // fallback: straight down

	for (const auto& light : scene.Lights())
	{
		if (light.LightType == Assets::LightProperties::Enum::DirectionalLight)
		{
			const glm::vec3 dir = glm::vec3(light.LightDir);
			if (glm::length(dir) > 0.0001f)
			{
				lightDir = glm::normalize(dir);
				break;
			}
		}
	}

	// ── 2. Compute world-space AABB from all scene objects' pivot positions ───
	// This ensures the shadow frustum covers the entire scene regardless of
	// how far objects are placed — critical when objects sit far from origin.
	constexpr float kBig = std::numeric_limits<float>::max();
	glm::vec3 sceneMin( kBig,  kBig,  kBig);
	glm::vec3 sceneMax(-kBig, -kBig, -kBig);

	for (GameObject* go : ModelManager::getInstance()->getObjectList())
	{
		if (!go) continue;
		// Extract world-space translation from column 3 of the world matrix
		const glm::vec3 pos = glm::vec3(go->getWorldMatrix()[3]);
		sceneMin = glm::min(sceneMin, pos);
		sceneMax = glm::max(sceneMax, pos);
	}

	// Fallback if scene is empty (or all pivots were degenerate)
	if (sceneMin.x > sceneMax.x)
	{
		sceneMin = glm::vec3(-100.0f);
		sceneMax = glm::vec3( 100.0f);
	}

	// Add a margin large enough to cover geometry extents beyond pivot points
	// (e.g. a sphere of radius 50 centred on its pivot, or a large mesh).
	constexpr float kMargin = 100.0f;
	sceneMin -= glm::vec3(kMargin);
	sceneMax += glm::vec3(kMargin);

	// Bounding sphere of the AABB — used to size the ortho frustum.
	const glm::vec3 sceneCenter = (sceneMin + sceneMax) * 0.5f;
	const float     sceneRadius = glm::length(sceneMax - sceneCenter);

	// ── 3. Build the light-camera matrices ────────────────────────────────────
	// Choose an up vector not collinear with lightDir
	const glm::vec3 up = (std::abs(lightDir.y) < 0.99f)
					   ? glm::vec3(0.0f, 1.0f, 0.0f)
					   : glm::vec3(1.0f, 0.0f, 0.0f);

	// Place the light camera outside the scene along the light direction
	const glm::vec3 eye  = sceneCenter - lightDir * sceneRadius;
	const glm::mat4 view = glm::lookAt(eye, sceneCenter, up);

	// Ortho frustum sized to the scene bounding sphere; near=0.1, far=diameter
	glm::mat4 proj = glm::ortho(
		-sceneRadius, sceneRadius,
		-sceneRadius, sceneRadius,
		0.1f, sceneRadius * 2.0f);

	// Flip Y: GLM was built for OpenGL (Y-up NDC); Vulkan's NDC has Y pointing down.
	proj[1][1] *= -1.0f;

	ShadowUBO ubo{};
	ubo.LightViewProj = proj * view;
	return ubo;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private — TransitionDepthImage
// ─────────────────────────────────────────────────────────────────────────────

void ShadowMapPass::TransitionDepthImage(
	VkCommandBuffer commandBuffer,
	VkImageLayout   oldLayout,
	VkImageLayout   newLayout) const
{
	VkImageMemoryBarrier barrier{};
	barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout           = oldLayout;
	barrier.newLayout           = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image               = shadowImage_->Handle();

	barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
	barrier.subresourceRange.baseMipLevel   = 0;
	barrier.subresourceRange.levelCount     = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount     = 1;

	VkPipelineStageFlags srcStage{};
	VkPipelineStageFlags dstStage{};

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		// First frame: image not yet used by anyone
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
							  | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
			 newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		// Subsequent frames: main pass is done reading; shadow pass will write
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
							  | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
			 newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		// Shadow pass is done; main pass will sample
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::runtime_error(
			"ShadowMapPass: unsupported depth image layout transition");
	}

	vkCmdPipelineBarrier(commandBuffer,
						 srcStage, dstStage,
						 0,
						 0, nullptr,
						 0, nullptr,
						 1, &barrier);
}

} // namespace Vulkan::Game
