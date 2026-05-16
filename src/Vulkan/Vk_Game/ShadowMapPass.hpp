#pragma once

#include "Vulkan/Vulkan.hpp"
#include "Utilities/Glm.hpp"  // MUST come before any other glm include — defines GLM_FORCE_DEPTH_ZERO_TO_ONE etc.
#include "ShadowMapSettings.hpp"
#include <memory>
#include <vector>

// ── Forward declarations ──────────────────────────────────────────────────────
namespace Assets { class Scene; }

namespace Vulkan
{
	class Buffer;
	class DescriptorSetManager;
	class Device;
	class DeviceMemory;
	class Image;
	class ImageView;
	class Sampler;
}

namespace Vulkan::Game
{
	/// @brief Depth-only shadow map pass supporting up to kMaxShadowLights
	///        simultaneous PCF directional-light shadow maps.
	///
	/// Creates and owns (per shadow slot):
	///  - A VK_FORMAT_D32_SFLOAT depth image + framebuffer.
	/// And globally:
	///  - One shared compare-enabled sampler (sampler2DShadow, LESS_OR_EQUAL).
	///  - A depth-only VkRenderPass + shadow graphics pipeline with depth bias.
	///  - Per-swapchain-frame ShadowUBO uniform buffers storing an array of
	///    orthographic light view-projection matrices and the active count.
	///
	/// Usage per frame (called from GameRenderer::Render):
	///   1. shadowMapPass_->UpdateLightVP(imageIndex, scene_)
	///        — collects all directional lights, fills ShadowUBO array.
	///   2. shadowMapPass_->Render(cmd, imageIndex, scene_)
	///        — records one depth sub-pass per active directional light.
	///      The main pass reads ShadowImageViews() / ShadowSampler() (binding 5,
	///      array of kMaxShadowLights) and LightVPBuffer() (binding 6, fragment).
	class ShadowMapPass final
	{
	public:
		VULKAN_NON_COPIABLE(ShadowMapPass)

		/// Maximum number of directional lights that can cast shadows simultaneously.
		/// Must match MAX_SHADOW_LIGHTS in shadow_vert.vert and game_frag.frag.
		static constexpr uint32_t kMaxShadowLights = 4;

		/// @brief CPU mirror of the GLSL ShadowUBO block (std140 compatible).
		struct ShadowUBO
		{
			glm::mat4 LightViewProj[kMaxShadowLights]{};
			uint32_t  Count{ 0 };
			float     _pad[3]{ 0.f, 0.f, 0.f }; // align to 16 bytes
		};

		/// @param device     Vulkan device — must outlive this object.
		/// @param imageCount Number of swapchain images (one UBO per frame).
		/// @param settings   Shadow map properties; defaults reproduce the original
		///                   2048² PCF configuration.
		ShadowMapPass(const Vulkan::Device& device,
					  uint32_t              imageCount,
					  ShadowMapSettings     settings = {});

		~ShadowMapPass();

		/// @brief Collect all directional lights from @p scene, compute their
		///        orthographic VP matrices, and upload into the per-frame ShadowUBO.
		///        Must be called before Render() for the same imageIndex.
		void UpdateLightVP(uint32_t imageIndex, const Assets::Scene& scene);

		/// @brief Record all shadow sub-passes into @p commandBuffer — one
		///        depth-only render pass per active directional light.
		///        Every shadow image is left in SHADER_READ_ONLY_OPTIMAL when done.
		void Render(VkCommandBuffer commandBuffer,
					uint32_t        imageIndex,
					const Assets::Scene& scene);

		// ── Accessors consumed by GameRenderer descriptor bindings ────────────

		/// @brief Returns VkImageViews for all kMaxShadowLights shadow depth images
		///        (binding 5 in main pass, descriptorCount = kMaxShadowLights).
		std::vector<VkImageView> ShadowImageViews() const;

		/// @brief Shared compare-enabled sampler (sampler2DShadow) for PCF.
		VkSampler ShadowSampler() const;

		/// @brief Per-frame ShadowUBO buffer (binding 6 in main pass, fragment).
		const Vulkan::Buffer& LightVPBuffer(uint32_t imageIndex) const;

		/// @brief Read-only access to the active shadow map settings.
		const ShadowMapSettings& Settings() const { return settings_; }

	private:
		/// @brief Internal per-slot depth image resources.
		struct ShadowLayer
		{
			std::unique_ptr<Vulkan::Image>        Image;
			std::unique_ptr<Vulkan::DeviceMemory> Memory;
			std::unique_ptr<Vulkan::ImageView>    ImageView;
			VkFramebuffer                         Framebuffer{ VK_NULL_HANDLE };
			mutable VkImageLayout                 CurrentLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
		};

		void CreateDepthResources();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateDescriptorSets(uint32_t imageCount);
		void CreatePipeline();
		glm::mat4 ComputeLightVP(glm::vec3        lightDir,
								 const glm::vec3& sceneMin,
								 const glm::vec3& sceneMax) const;

		/// @brief Emit a VkImageMemoryBarrier for the depth image in @p layer.
		void TransitionDepthImage(VkCommandBuffer commandBuffer,
								  ShadowLayer&    layer,
								  VkImageLayout   newLayout) const;

		// ── Non-owning reference ──────────────────────────────────────────────
		const Vulkan::Device& device_;

		// ── Shadow map properties ─────────────────────────────────────────────
		ShadowMapSettings settings_;

		// ── Per-slot depth image resources ───────────────────────────────────
		// Fixed array of kMaxShadowLights; all slots are created up-front so
		// descriptor binding 5 always has valid images (unused slots are cleared
		// to 1.0 = fully lit on first use).
		std::vector<ShadowLayer>          layers_;

		// Shared sampler — all shadow maps use the same compare / filter config.
		std::unique_ptr<Vulkan::Sampler>  shadowSampler_;

		// Cached count of active directional lights from the last UpdateLightVP call.
		uint32_t activeLightCount_{ 0 };

		// ── Depth-only render pass (shared by all sub-passes) ─────────────────
		VkRenderPass renderPass_{ VK_NULL_HANDLE };

		// ── Shadow pipeline (vertex-only, depth bias enabled) ─────────────────
		VkPipelineLayout pipelineLayout_{ VK_NULL_HANDLE };
		VkPipeline       pipeline_      { VK_NULL_HANDLE };

		// ── Per-frame ShadowUBO resources ─────────────────────────────────────
		std::unique_ptr<Vulkan::DescriptorSetManager>      descriptorSetManager_;
		std::vector<std::unique_ptr<Vulkan::Buffer>>       lightVPBuffers_;
		std::vector<Vulkan::DeviceMemory>                  lightVPMemories_;
	};

} // namespace Vulkan::Game
