#include "GameRenderer.hpp"
#include "ShadowMapPass.hpp"
#include "PointLightShadowPass.hpp"
#include "IBL/IBLPrecompute.hpp"

#include <algorithm>

#include "Assets/Scene.hpp"
#include "Assets/UniformBuffer.hpp"
#include "Assets/Vertex.hpp"
#include "From-GDGRAP2/ModelManager.h"
#include "From-GDGRAP2/GameObject.h"
#include "Assets/Model.hpp"
#include "Utilities/Exception.hpp"
#include "Utilities/FileUtils.h"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/BufferUtil.hpp"
#include "Vulkan/CommandPool.hpp"
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
	const Assets::Scene& scene,
	Vulkan::CommandPool& commandPool) :
	swapChain_(swapChain),
	depthBuffer_(depthBuffer),
	scene_(scene),
	uniformBuffers_(&uniformBuffers),
	commandPool_(&commandPool)
{
	CreateRenderPass();
	shadowMapPass_ = std::make_unique<ShadowMapPass>(
		swapChain.Device(),
		static_cast<uint32_t>(uniformBuffers.size()));
	pointLightShadowPass_ = std::make_unique<PointLightShadowPass>(
		swapChain.Device(),
		static_cast<uint32_t>(uniformBuffers.size()));

	// Pre-compute IBL textures if the scene has a skybox
	if (scene.SkyboxImageView() != VK_NULL_HANDLE)
	{
		iblPrecompute_ = std::make_unique<IBLPrecompute>(
			swapChain.Device(),
			commandPool,
			scene.SkyboxImageView(),
			scene.SkyboxSampler());
	}

	CreateGameRendererMaterialPropsBuffer(scene);

	CreateDescriptorSets(uniformBuffers, scene);

	EventBroadcaster::getInstance()->addObserver(
		EventNames::ON_SHADOW_SETTINGS_CHANGED, this);
	CreatePipeline();
	CreateFramebuffers();
}

GameRenderer::~GameRenderer()
{
	EventBroadcaster::getInstance()->removeObserver(
		EventNames::ON_SHADOW_SETTINGS_CHANGED);

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
	pointLightShadowPass_.reset();
	shadowMapPass_.reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// Shadow settings hot-reload
// ─────────────────────────────────────────────────────────────────────────────

void GameRenderer::ApplyShadowSettings(ShadowMapSettings settings)
{
	// GPU must be idle before we destroy the old shadow resources.
	vkDeviceWaitIdle(swapChain_.Device().Handle());

	// Tear down the old pass + descriptor sets that reference its images.
	descriptorSetManager_.reset();
	shadowMapPass_.reset();

	// Rebuild with the new settings.
	shadowMapPass_ = std::make_unique<ShadowMapPass>(
		swapChain_.Device(),
		static_cast<uint32_t>(uniformBuffers_->size()),
		std::move(settings));

	CreateDescriptorSets(*uniformBuffers_, scene_);
}

const ShadowMapSettings& GameRenderer::GetShadowSettings() const
{
	return shadowMapPass_->Settings();
}

void GameRenderer::onTriggeredEvent(std::string eventName,
									std::shared_ptr<Parameters> parameters)
{
	if (eventName == EventNames::ON_SHADOW_SETTINGS_CHANGED && parameters)
	{
		// Store the new settings and raise the pending flag.
		// The ACTUAL Vulkan recreation is deferred to FlushPendingShadowReload(),
		// which must be called between frames (before commandBuffers_->Begin) so
		// that we never touch GPU resources while a command buffer is recording.
		auto* rawSettings = static_cast<ShadowMapSettings*>(
			parameters->getHandleData("settings", nullptr));
		if (rawSettings)
		{
			pendingShadowSettings_ = *rawSettings;
			pendingShadowReload_   = true;
		}
	}
}

void GameRenderer::FlushPendingShadowReload()
{
	if (!pendingShadowReload_) return;
	pendingShadowReload_ = false;
	ApplyShadowSettings(std::move(pendingShadowSettings_));
}



VkDescriptorSet GameRenderer::DescriptorSet(const uint32_t index) const
{
	return descriptorSetManager_->DescriptorSets().Handle(index);
}

void GameRenderer::Render(VkCommandBuffer commandBuffer, const uint32_t imageIndex)
{
	// ── Shadow passes (depth-only, runs before the main forward pass) ──────────
	// Directional light shadows
	shadowMapPass_->UpdateLightVP(imageIndex, scene_);
	shadowMapPass_->Render(commandBuffer, imageIndex, scene_);

	// Point light shadows (cubemaps)
	pointLightShadowPass_->UpdateLightVP(imageIndex, scene_);
	pointLightShadowPass_->Render(commandBuffer, imageIndex, scene_);

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

			// Skip purely emissive meshes (ray-tracer area lights — DiffuseLight material).
			// They are not PBR light sources; rendering them in the Game Renderer would
			// just show a bright flat box that confuses the scene.
			{
				const auto& mats = go->getModel()->Materials();
				const bool allEmissive = !mats.empty() && std::all_of(
					mats.begin(), mats.end(),
					[](const Assets::Material& m){ return m.MaterialModel == Assets::Material::Enum::DiffuseLight; });
				if (allEmissive) { indexOffset += static_cast<uint32_t>(go->getModel()->NumberOfIndices()); vertexOffset += static_cast<uint32_t>(go->getModel()->NumberOfVertices()); continue; }
			}

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

void GameRenderer::CreateGameRendererMaterialPropsBuffer(const Assets::Scene& scene)
{
	// Calculate the number of materials from the material buffer size
	const size_t materialBufferSize = scene.MaterialBuffer().Size();
	const size_t materialCount = materialBufferSize / sizeof(Assets::Material);

	// Create default properties for each material
	std::vector<Vulkan::Game::GameRendererMaterialProperties> properties;

	// Always create at least 1 element even if materialCount is 0
	// This prevents issues with empty buffers when no materials exist
	const size_t minimumCount = std::max(size_t(1), materialCount);
	properties.reserve(minimumCount);

	for (size_t i = 0; i < minimumCount; ++i)
	{
		Vulkan::Game::GameRendererMaterialProperties prop{};
		// Default initialization: no maps, use material defaults
		prop.NormalMapTextureId = -1;
		prop.NormalMapStrength = 1.0f;
		prop.MetallicMapTextureId = -1;
		prop.MetallicValue = 0.0f;
		prop.RoughnessMapTextureId = -1;
		prop.RoughnessValue = 0.5f;
		prop.AOMapTextureId = -1;
		prop.AOStrength = 1.0f;

		// ===== NEW: Alpha fields initialization =====
		// Note: Alpha properties are now part of the base Material struct.
		// When scene materials are loaded with SetTransparent(), they will have
		// AlphaBlendMode, AlphaCutoffThreshold, and AlphaMapTextureId set.
		// However, the GameRendererMaterialProperties buffer is independent and
		// initialized with safe defaults here. To actually use transparency,
		// update the material loading pipeline to populate these fields correctly.
		prop.AlphaMapTextureId = -1;      // No dedicated alpha map by default
		prop.AlphaCutoffThreshold = 0.5f; // Standard cutoff threshold
		prop.AlphaBlendMode = 0u;         // 0 = Opaque (no blending)

		properties.push_back(prop);
	}

	// Use BufferUtil to create a GPU-resident buffer with staging
	Vulkan::BufferUtil::CreateDeviceBuffer(
		*commandPool_,
		"GameRendererMaterialProperties",
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		properties,
		gameRendererMatPropsBuffer_,
		gameRendererMatPropsBufferMemory_);
}

void GameRenderer::CreateDescriptorSets(
	const std::vector<Assets::UniformBuffer>& uniformBuffers,
	const Assets::Scene& scene)
{
	const auto& device = swapChain_.Device();

	// Ensure device is idle before updating descriptors (critical when switching renderers)
	device.WaitIdle();

	// binding 0 : UBO              (UNIFORM_BUFFER,         vert + frag)
	// binding 1 : Material buffer  (STORAGE_BUFFER,         frag)
	// binding 2 : Light buffer     (STORAGE_BUFFER,         frag)
	// binding 3 : Texture array    (COMBINED_IMAGE_SAMPLER, frag) — omitted when scene has no textures
	// binding 4 : Skybox sampler   (COMBINED_IMAGE_SAMPLER, frag)
	const uint32_t texCount = static_cast<uint32_t>(scene.TextureSamplers().size());

	// Build binding list conditionally: binding 3 is only added when the scene actually
	// has textures. A descriptorCount of 0 violates the Vulkan spec
	// (VUID-VkDescriptorPoolSize-descriptorCount-00302) and corrupts pool tracking.
	std::vector<DescriptorBinding> bindings =
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
	};

	if (texCount > 0)
	{
		bindings.push_back({3, texCount,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT});
	}

	bindings.push_back({4, 1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT});

	// binding 5 : shadow maps (COMBINED_IMAGE_SAMPLER with compare, frag) — array of kMaxShadowLights
	bindings.push_back({5, ShadowMapPass::kMaxShadowLights,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT});

	// binding 6 : ShadowUBO   (UNIFORM_BUFFER, frag — array of light VP matrices)
	bindings.push_back({6, 1,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_FRAGMENT_BIT});

	// binding 7 : point light shadow cubemaps (COMBINED_IMAGE_SAMPLER with compare, frag)
	bindings.push_back({7, PointLightShadowPass::kMaxPointShadowLights,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT});

	// binding 8 : PointShadowUBO (UNIFORM_BUFFER, frag — cubemap VP matrices)
	bindings.push_back({8, 1,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_SHADER_STAGE_FRAGMENT_BIT});

	// binding 9  : IBL irradiance cubemap    (COMBINED_IMAGE_SAMPLER, frag)
	bindings.push_back({9, 1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT});

	// binding 10 : IBL prefiltered env cubemap (COMBINED_IMAGE_SAMPLER, frag)
	bindings.push_back({10, 1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT});

	// binding 11 : BRDF integration LUT       (COMBINED_IMAGE_SAMPLER, frag)
	bindings.push_back({11, 1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_SHADER_STAGE_FRAGMENT_BIT});

	// ═══ NEW BINDINGS FOR NORMAL MAPPING & PBR (Game Renderer only) ═══
	// All texture map bindings (12-16) are only added when the scene has textures.
	// When texCount == 0:
	//   - No textures exist, so material texture IDs are all -1 (checked in shader)
	//   - Bindings 12-16 are simply not used
	//   - Binding 13 is omitted from descriptor set layout
	// When bindings are omitted, shader function calls gracefully handle it because
	// the texture ID checks (matProps.NormalMapTextureId < 0) will always be true.

	if (texCount > 0)
	{
		// binding 12 : Normal maps array     (COMBINED_IMAGE_SAMPLER, frag)
		bindings.push_back({12, texCount,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT});

		// binding 13 : Material properties buffer (STORAGE_BUFFER, frag)
		// Contains per-material texture IDs and strength values
		bindings.push_back({13, 1,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			VK_SHADER_STAGE_FRAGMENT_BIT});

		// binding 14 : Metallic maps array   (COMBINED_IMAGE_SAMPLER, frag)
		bindings.push_back({14, texCount,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT});

		// binding 15 : Roughness maps array  (COMBINED_IMAGE_SAMPLER, frag)
		bindings.push_back({15, texCount,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT});

		// binding 16 : AO maps array         (COMBINED_IMAGE_SAMPLER, frag)
		bindings.push_back({16, texCount,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT});

		// binding 17 : Alpha maps array      (COMBINED_IMAGE_SAMPLER, frag)
		// Dedicated alpha/transparency textures for alpha cutoff and transparency blending
		bindings.push_back({17, texCount,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT});
	}

	descriptorSetManager_.reset(new DescriptorSetManager(device, bindings, uniformBuffers.size()));
	auto& descriptorSets = descriptorSetManager_->DescriptorSets();

	for (uint32_t i = 0; i < static_cast<uint32_t>(swapChain_.Images().size()); ++i)
	{
		VkDescriptorBufferInfo uboInfo{};
		uboInfo.buffer = uniformBuffers[i].Buffer().Handle();
		uboInfo.range  = VK_WHOLE_SIZE;
		if (uboInfo.buffer == VK_NULL_HANDLE)
		{
			Throw(std::runtime_error("UBO buffer handle is null when creating descriptor sets"));
		}

		VkDescriptorBufferInfo materialInfo{};
		materialInfo.buffer = scene.MaterialBuffer().Handle();
		materialInfo.range  = VK_WHOLE_SIZE;
		if (materialInfo.buffer == VK_NULL_HANDLE)
		{
			Throw(std::runtime_error("Material buffer handle is null when creating descriptor sets"));
		}

		VkDescriptorBufferInfo lightsInfo{};
		lightsInfo.buffer = scene.LightBuffer().Handle();
		lightsInfo.range  = VK_WHOLE_SIZE;
		if (lightsInfo.buffer == VK_NULL_HANDLE)
		{
			Throw(std::runtime_error("Lights buffer handle is null when creating descriptor sets"));
		}

		// ─── CRITICAL: Keep all image info vectors in scope until UpdateDescriptors ───
		// The Bind() function stores pointers to imageInfo data in VkWriteDescriptorSet.
		// If these vectors go out of scope before UpdateDescriptors(), we'll have stale pointers!
		std::vector<VkDescriptorImageInfo> textureInfos;
		std::vector<VkDescriptorImageInfo> shadowInfos;
		std::vector<VkDescriptorImageInfo> pointShadowInfos;
		std::vector<VkDescriptorImageInfo> normalMapInfos;
		std::vector<VkDescriptorImageInfo> metallicMapInfos;
		std::vector<VkDescriptorImageInfo> roughnessMapInfos;
		std::vector<VkDescriptorImageInfo> aoMapInfos;
		std::vector<VkDescriptorImageInfo> alphaMapInfos;

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

		std::vector<VkWriteDescriptorSet> writes =
		{
			descriptorSets.Bind(i, 0, uboInfo),
			descriptorSets.Bind(i, 1, materialInfo),
			descriptorSets.Bind(i, 2, lightsInfo),
		};

		// Binding 3 is only written when the scene has textures.
		// Dereferencing textureInfos.data() on an empty vector is UB.
		if (!textureInfos.empty())
		{
			writes.push_back(descriptorSets.Bind(i, 3,
				*textureInfos.data(),
				static_cast<uint32_t>(textureInfos.size())));
		}

		writes.push_back(descriptorSets.Bind(i, 4, skyboxInfo));

		// Binding 5: shadow depth maps — one sampler per slot (kMaxShadowLights total).
		// Unused slots are filled with the first shadow map's view so every descriptor
		// slot is valid; the fragment shader only samples slots [0, Count).
		const std::vector<VkImageView> shadowViews = shadowMapPass_->ShadowImageViews();
		const VkSampler                shadowSampler = shadowMapPass_->ShadowSampler();

		shadowInfos.reserve(ShadowMapPass::kMaxShadowLights);
		for (uint32_t s = 0; s < ShadowMapPass::kMaxShadowLights; ++s)
		{
			VkDescriptorImageInfo si{};
			si.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			si.imageView   = shadowViews[s]; // all kMaxShadowLights slots are always valid
			si.sampler     = shadowSampler;
			shadowInfos.push_back(si);
		}
		writes.push_back(descriptorSets.Bind(i, 5,
			*shadowInfos.data(),
			static_cast<uint32_t>(shadowInfos.size())));

		// Binding 6: ShadowUBO (light view-projection, vertex stage)
		VkDescriptorBufferInfo shadowUboInfo{};
		shadowUboInfo.buffer = shadowMapPass_->LightVPBuffer(i).Handle();
		shadowUboInfo.offset = 0;
		shadowUboInfo.range  = VK_WHOLE_SIZE;
		writes.push_back(descriptorSets.Bind(i, 6, shadowUboInfo));

		// Binding 7: point light shadow cubemaps — one sampler per point light slot
		const std::vector<VkImageView> pointShadowViews = pointLightShadowPass_->PointShadowImageViews();
		const VkSampler                pointShadowSampler = pointLightShadowPass_->PointShadowSampler();

		pointShadowInfos.reserve(PointLightShadowPass::kMaxPointShadowLights);
		for (uint32_t ps = 0; ps < PointLightShadowPass::kMaxPointShadowLights; ++ps)
		{
			VkDescriptorImageInfo psi{};
			psi.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			psi.imageView   = pointShadowViews[ps]; // all slots are always valid
			psi.sampler     = pointShadowSampler;
			pointShadowInfos.push_back(psi);
		}
		writes.push_back(descriptorSets.Bind(i, 7,
			*pointShadowInfos.data(),
			static_cast<uint32_t>(pointShadowInfos.size())));

		// Binding 8: PointShadowUBO (point light cubemap VP matrices, fragment stage)
		VkDescriptorBufferInfo pointShadowUboInfo{};
		pointShadowUboInfo.buffer = pointLightShadowPass_->PointLightVPBuffer(i).Handle();
		pointShadowUboInfo.offset = 0;
		pointShadowUboInfo.range  = VK_WHOLE_SIZE;
		writes.push_back(descriptorSets.Bind(i, 8, pointShadowUboInfo));

		// Bindings 9/10/11: IBL textures — use real IBL when available,
		// otherwise bind the skybox itself as a harmless dummy so every
		// descriptor slot remains valid. The shader branches on ubo.HasSky.
		const VkImageView  iblFallbackView    = scene.SkyboxImageView();
		const VkSampler    iblFallbackSampler = scene.SkyboxSampler();

		VkDescriptorImageInfo iblIrrInfo{};
		iblIrrInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		iblIrrInfo.imageView   = iblPrecompute_ ? iblPrecompute_->IrradianceView()    : iblFallbackView;
		iblIrrInfo.sampler     = iblPrecompute_ ? iblPrecompute_->IrradianceSampler() : iblFallbackSampler;
		writes.push_back(descriptorSets.Bind(i, 9, iblIrrInfo));

		VkDescriptorImageInfo iblPreInfo{};
		iblPreInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		iblPreInfo.imageView   = iblPrecompute_ ? iblPrecompute_->PrefilteredView()    : iblFallbackView;
		iblPreInfo.sampler     = iblPrecompute_ ? iblPrecompute_->PrefilteredSampler() : iblFallbackSampler;
		writes.push_back(descriptorSets.Bind(i, 10, iblPreInfo));

		VkDescriptorImageInfo iblLutInfo{};
		iblLutInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		iblLutInfo.imageView   = iblPrecompute_ ? iblPrecompute_->BrdfLutView()    : iblFallbackView;
		iblLutInfo.sampler     = iblPrecompute_ ? iblPrecompute_->BrdfLutSampler() : iblFallbackSampler;
		writes.push_back(descriptorSets.Bind(i, 11, iblLutInfo));

		// ═══ NEW DESCRIPTOR WRITES FOR NORMAL MAPPING (Bindings 12-16) ═══
		// Only written when the scene has textures (texCount > 0).
		if (texCount > 0)
		{
			// Validate gameRendererMatPropsBuffer_ exists before using it
			if (!gameRendererMatPropsBuffer_)
			{
				Throw(std::runtime_error("Game renderer material properties buffer was not initialized"));
			}

			// Binding 12: Normal maps — reuse the same texture array, just another view
			normalMapInfos.reserve(scene.TextureSamplers().size());
			for (size_t t = 0; t < scene.TextureSamplers().size(); ++t)
			{
				VkDescriptorImageInfo nmi{};
				nmi.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				nmi.imageView   = scene.TextureImageViews()[t];
				nmi.sampler     = scene.TextureSamplers()[t];
				normalMapInfos.push_back(nmi);
			}
			writes.push_back(descriptorSets.Bind(i, 12,
				*normalMapInfos.data(),
				static_cast<uint32_t>(normalMapInfos.size())));

			// Binding 13: Material properties buffer (Game Renderer-specific)
			VkDescriptorBufferInfo matPropsInfo{};
			matPropsInfo.buffer = gameRendererMatPropsBuffer_->Handle();
			matPropsInfo.offset = 0;
			matPropsInfo.range  = VK_WHOLE_SIZE;
			if (matPropsInfo.buffer == VK_NULL_HANDLE)
			{
				Throw(std::runtime_error("Material properties buffer handle is null"));
			}
			writes.push_back(descriptorSets.Bind(i, 13, matPropsInfo));

			// Binding 14: Metallic maps — same texture array
			metallicMapInfos.reserve(scene.TextureSamplers().size());
			for (size_t t = 0; t < scene.TextureSamplers().size(); ++t)
			{
				VkDescriptorImageInfo mmi{};
				mmi.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				mmi.imageView   = scene.TextureImageViews()[t];
				mmi.sampler     = scene.TextureSamplers()[t];
				metallicMapInfos.push_back(mmi);
			}
			writes.push_back(descriptorSets.Bind(i, 14,
				*metallicMapInfos.data(),
				static_cast<uint32_t>(metallicMapInfos.size())));

			// Binding 15: Roughness maps — same texture array
			roughnessMapInfos.reserve(scene.TextureSamplers().size());
			for (size_t t = 0; t < scene.TextureSamplers().size(); ++t)
			{
				VkDescriptorImageInfo rmi{};
				rmi.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				rmi.imageView   = scene.TextureImageViews()[t];
				rmi.sampler     = scene.TextureSamplers()[t];
				roughnessMapInfos.push_back(rmi);
			}
			writes.push_back(descriptorSets.Bind(i, 15,
				*roughnessMapInfos.data(),
				static_cast<uint32_t>(roughnessMapInfos.size())));

			// Binding 16: AO maps — same texture array
			aoMapInfos.reserve(scene.TextureSamplers().size());
			for (size_t t = 0; t < scene.TextureSamplers().size(); ++t)
			{
				VkDescriptorImageInfo ami{};
				ami.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				ami.imageView   = scene.TextureImageViews()[t];
				ami.sampler     = scene.TextureSamplers()[t];
				aoMapInfos.push_back(ami);
			}
			writes.push_back(descriptorSets.Bind(i, 16,
				*aoMapInfos.data(),
				static_cast<uint32_t>(aoMapInfos.size())));

			// Binding 17: Alpha maps — dedicated alpha/transparency textures
			// Same texture array as other map types; can be reused or contain dedicated alpha-only content
			alphaMapInfos.reserve(scene.TextureSamplers().size());
			for (size_t t = 0; t < scene.TextureSamplers().size(); ++t)
			{
				VkDescriptorImageInfo ami{};
				ami.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				ami.imageView   = scene.TextureImageViews()[t];
				ami.sampler     = scene.TextureSamplers()[t];
				alphaMapInfos.push_back(ami);
			}
			writes.push_back(descriptorSets.Bind(i, 17,
				*alphaMapInfos.data(),
				static_cast<uint32_t>(alphaMapInfos.size())));
		}

		// ─── CRITICAL: All vectors stay in scope until after this call ───
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
