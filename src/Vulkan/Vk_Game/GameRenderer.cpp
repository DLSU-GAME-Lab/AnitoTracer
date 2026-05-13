#include "GameRenderer.hpp"

#include "Assets/Scene.hpp"
#include "Assets/UniformBuffer.hpp"
#include "Assets/Vertex.hpp"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/GameObject.h"
#include "Assets/Model.hpp"
#include "Utilities/Exception.hpp"
#include "Utilities/FileUtils.h"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/DepthBuffer.hpp"
#include "Vulkan/DescriptorBinding.hpp"
#include "Vulkan/DescriptorSetManager.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/DescriptorSets.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/SwapChain.hpp"

namespace Vulkan::Game {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

GameRenderer::GameRenderer(
	const Vulkan::SwapChain& swapChain,
	const Vulkan::DepthBuffer& depthBuffer,
	const std::vector<Assets::UniformBuffer>& uniformBuffers,
	const Assets::Scene& scene) :
	swapChain_(swapChain),
	depthBuffer_(depthBuffer),
	scene_(scene)
{
	CreateRenderPass();
	CreateDescriptorSets(uniformBuffers, scene);
	CreatePipeline();
	CreateFramebuffers();
}

GameRenderer::~GameRenderer()
{
	// Framebuffers first — they reference the render pass
	for (VkFramebuffer fb : framebuffers_)
	{
		if (fb != VK_NULL_HANDLE)
			vkDestroyFramebuffer(swapChain_.Device().Handle(), fb, nullptr);
	}
	framebuffers_.clear();

	if (pipeline_ != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(swapChain_.Device().Handle(), pipeline_, nullptr);
		pipeline_ = VK_NULL_HANDLE;
	}

	if (pipelineLayoutRaw_ != VK_NULL_HANDLE)
	{
		vkDestroyPipelineLayout(swapChain_.Device().Handle(), pipelineLayoutRaw_, nullptr);
		pipelineLayoutRaw_ = VK_NULL_HANDLE;
	}

	descriptorSetManager_.reset();
	renderPass_.reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public interface
// ─────────────────────────────────────────────────────────────────────────────

VkDescriptorSet GameRenderer::DescriptorSet(const uint32_t index) const
{
	return descriptorSetManager_->DescriptorSets().Handle(index);
}

void GameRenderer::Render(VkCommandBuffer commandBuffer, const uint32_t imageIndex)
{
	// ── Begin render pass ─────────────────────────────────────────────────────
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color        = { { 0.05f, 0.05f, 0.05f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass        = renderPass_->Handle();
	renderPassInfo.framebuffer       = framebuffers_[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChain_.Extent();
	renderPassInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues      = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

		VkDescriptorSet ds = DescriptorSet(imageIndex);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
								pipelineLayoutRaw_, 0, 1, &ds, 0, nullptr);

		// Bind the scene's packed vertex + index buffers
		VkBuffer     vertexBuffers[] = { scene_.VertexBuffer().Handle() };
		VkDeviceSize offsets[]       = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, scene_.IndexBuffer().Handle(), 0, VK_INDEX_TYPE_UINT32);

		// Draw each GameObject with its model matrix pushed as a push constant
		uint32_t vertexOffset = 0;
		uint32_t indexOffset  = 0;

		for (GameObject* go : ModelManager::getInstance()->getObjectList())
		{
			if (!go || !go->getModel()) continue;

			glm::mat4 worldMatrix = go->getWorldMatrix();
				vkCmdPushConstants(commandBuffer, pipelineLayoutRaw_,
							   VK_SHADER_STAGE_VERTEX_BIT, 0,
							   sizeof(glm::mat4), &worldMatrix);

			const uint32_t indexCount  = static_cast<uint32_t>(go->getModel()->NumberOfIndices());
			const uint32_t vertexCount = static_cast<uint32_t>(go->getModel()->NumberOfVertices());

			vkCmdDrawIndexed(commandBuffer, indexCount, 1,
							 indexOffset, static_cast<int32_t>(vertexOffset), 0);

			vertexOffset += vertexCount;
			indexOffset  += indexCount;
		}
	}
	vkCmdEndRenderPass(commandBuffer);
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void GameRenderer::CreateRenderPass()
{
	// Reuse the existing RenderPass wrapper with CLEAR load ops for both
	// color (swapchain) and depth attachments.
	renderPass_.reset(new Vulkan::RenderPass(
		swapChain_,
		depthBuffer_,
		VK_ATTACHMENT_LOAD_OP_CLEAR,   // color — clear to background every frame
		VK_ATTACHMENT_LOAD_OP_CLEAR)); // depth — reset depth buffer every frame
}

void GameRenderer::CreateDescriptorSets(
	const std::vector<Assets::UniformBuffer>& uniformBuffers,
	const Assets::Scene& scene)
{
	const auto& device = swapChain_.Device();

	// binding 0 : UBO              (UNIFORM_BUFFER,         vert + frag)
	// binding 1 : Material buffer  (STORAGE_BUFFER,         frag)
	// binding 2 : Light buffer     (STORAGE_BUFFER,         frag)
	// binding 3 : Texture array    (COMBINED_IMAGE_SAMPLER, frag)
	// binding 4 : Skybox sampler   (COMBINED_IMAGE_SAMPLER, frag)
	const std::vector<DescriptorBinding> bindings =
	{
		{0, 1,
		 VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		 VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},

		{1, 1,
		 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		 VK_SHADER_STAGE_FRAGMENT_BIT},

		{2, 1,
		 VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		 VK_SHADER_STAGE_FRAGMENT_BIT},

		{3, static_cast<uint32_t>(scene.TextureSamplers().size()),
		 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		 VK_SHADER_STAGE_FRAGMENT_BIT},

		{4, 1,
		 VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		 VK_SHADER_STAGE_FRAGMENT_BIT},
	};

	descriptorSetManager_.reset(new DescriptorSetManager(device, bindings, uniformBuffers.size()));
	auto& descriptorSets = descriptorSetManager_->DescriptorSets();

	for (uint32_t i = 0; i < static_cast<uint32_t>(swapChain_.Images().size()); ++i)
	{
		VkDescriptorBufferInfo uboInfo{};
		uboInfo.buffer = uniformBuffers[i].Buffer().Handle();
		uboInfo.range  = VK_WHOLE_SIZE;

		VkDescriptorBufferInfo materialInfo{};
		materialInfo.buffer = scene.MaterialBuffer().Handle();
		materialInfo.range  = VK_WHOLE_SIZE;

		VkDescriptorBufferInfo lightsInfo{};
		lightsInfo.buffer = scene.LightBuffer().Handle();
		lightsInfo.range  = VK_WHOLE_SIZE;

		std::vector<VkDescriptorImageInfo> textureInfos;
		textureInfos.reserve(scene.TextureSamplers().size());
		for (size_t t = 0; t < scene.TextureSamplers().size(); ++t)
		{
			VkDescriptorImageInfo ti{};
			ti.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			ti.imageView   = scene.TextureImageViews()[t];
			ti.sampler     = scene.TextureSamplers()[t];
			textureInfos.push_back(ti);
		}

		VkDescriptorImageInfo skyboxInfo{};
		skyboxInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		skyboxInfo.imageView   = scene.SkyboxImageView();
		skyboxInfo.sampler     = scene.SkyboxSampler();

		const std::vector<VkWriteDescriptorSet> writes =
		{
			descriptorSets.Bind(i, 0, uboInfo),
			descriptorSets.Bind(i, 1, materialInfo),
			descriptorSets.Bind(i, 2, lightsInfo),
			descriptorSets.Bind(i, 3, *textureInfos.data(),
								static_cast<uint32_t>(textureInfos.size())),
			descriptorSets.Bind(i, 4, skyboxInfo),
		};

		descriptorSets.UpdateDescriptors(i, writes);
	}

	// Pipeline layout: descriptor set + push constant for per-object mat4 model matrix
	VkPushConstantRange pushRange{};
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushRange.offset     = 0;
	pushRange.size       = sizeof(glm::mat4);

	const VkDescriptorSetLayout dsl = descriptorSetManager_->DescriptorSetLayout().Handle();

	VkPipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount         = 1;
	layoutInfo.pSetLayouts            = &dsl;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pPushConstantRanges    = &pushRange;

	Check(vkCreatePipelineLayout(device.Handle(), &layoutInfo, nullptr, &pipelineLayoutRaw_),
		  "create Game renderer pipeline layout");
}

void GameRenderer::CreatePipeline()
{
	const auto& device = swapChain_.Device();

	// ── Vertex input: reuse Assets::Vertex attribute layout ──────────────────
	const auto bindingDesc    = Assets::Vertex::GetBindingDescription();
	const auto attributeDescs = Assets::Vertex::GetAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount   = 1;
	vertexInput.pVertexBindingDescriptions      = &bindingDesc;
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescs.size());
	vertexInput.pVertexAttributeDescriptions    = attributeDescs.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// ── Viewport & scissor (match swapchain extent) ───────────────────────────
	VkViewport viewport{};
	viewport.x        = 0.0f;
	viewport.y        = 0.0f;
	viewport.width    = static_cast<float>(swapChain_.Extent().width);
	viewport.height   = static_cast<float>(swapChain_.Extent().height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain_.Extent();

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports    = &viewport;
	viewportState.scissorCount  = 1;
	viewportState.pScissors     = &scissor;

	// ── Rasterizer ────────────────────────────────────────────────────────────
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable        = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth               = 1.0f;
	rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable         = VK_FALSE;

	// ── Multisampling (disabled — MSAA can be added in a later phase) ─────────
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable  = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// ── Depth / stencil ───────────────────────────────────────────────────────
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable  = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS;

	// ── Color blend: opaque pass, no blending ─────────────────────────────────
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable   = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments    = &colorBlendAttachment;

	// ── Load shaders ─────────────────────────────────────────────────────────
	const std::string shaderDir = FileUtils::getAssetsFolderPath().generic_string() + "/shaders/";
	const ShaderModule vertShader(device, shaderDir + "game_vert.vert.spv");
	const ShaderModule fragShader(device, shaderDir + "game_frag.frag.spv");

	VkPipelineShaderStageCreateInfo shaderStages[] =
	{
		vertShader.CreateShaderStage(VK_SHADER_STAGE_VERTEX_BIT),
		fragShader.CreateShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT),
	};

	// ── Assemble graphics pipeline ────────────────────────────────────────────
	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount          = 2;
	pipelineInfo.pStages             = shaderStages;
	pipelineInfo.pVertexInputState   = &vertexInput;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState      = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState   = &multisampling;
	pipelineInfo.pDepthStencilState  = &depthStencil;
	pipelineInfo.pColorBlendState    = &colorBlending;
	pipelineInfo.layout              = pipelineLayoutRaw_;
	pipelineInfo.renderPass          = renderPass_->Handle();
	pipelineInfo.subpass             = 0;

	Check(vkCreateGraphicsPipelines(device.Handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_),
		  "create Game renderer graphics pipeline");
}

void GameRenderer::CreateFramebuffers()
{
	const auto& device     = swapChain_.Device();
	const auto& imageViews = swapChain_.ImageViews();
	const auto  extent     = swapChain_.Extent();

	framebuffers_.resize(imageViews.size(), VK_NULL_HANDLE);

	for (size_t i = 0; i < imageViews.size(); ++i)
	{
		// Each framebuffer attaches: [0] swapchain color view, [1] depth view
		std::array<VkImageView, 2> attachments =
		{
			imageViews[i]->Handle(),
			depthBuffer_.ImageView().Handle()
		};

		VkFramebufferCreateInfo fbInfo{};
		fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbInfo.renderPass      = renderPass_->Handle();
		fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		fbInfo.pAttachments    = attachments.data();
		fbInfo.width           = extent.width;
		fbInfo.height          = extent.height;
		fbInfo.layers          = 1;

		Check(vkCreateFramebuffer(device.Handle(), &fbInfo, nullptr, &framebuffers_[i]),
			  "create Game renderer framebuffer");
	}
}

} // namespace Vulkan::Game
