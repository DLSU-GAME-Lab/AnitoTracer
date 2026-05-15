#pragma once

#include "Vulkan/Vulkan.hpp"
#include "Utilities/Glm.hpp"  // MUST come before any other glm include — defines GLM_FORCE_DEPTH_ZERO_TO_ONE etc.
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
	/// @brief Depth-only shadow map pass for PCF directional-light shadows.
	///
	/// Creates and owns:
	///  - A 2048×2048 VK_FORMAT_D32_SFLOAT depth image + a compare-enabled
	///    sampler (sampler2DShadow, LESS_OR_EQUAL) suitable for PCF reads.
	///  - A depth-only VkRenderPass + VkFramebuffer.
	///  - A shadow graphics pipeline (vertex shader only) with depth bias to
	///    prevent self-shadowing acne.
	///  - Per-swapchain-frame ShadowUBO uniform buffers that store the
	///    orthographic light view-projection matrix.
	///
	/// Usage per frame (called from GameRenderer::Render):
	///   1. shadowMapPass_->UpdateLightVP(imageIndex, scene_)  — upload mat4
	///   2. shadowMapPass_->Render(cmd, imageIndex, scene_)    — record shadow cmds
	///      The main pass then reads ShadowImageView() / ShadowSampler() (binding 5)
	///      and LightVPBuffer() (binding 6) for PCF shadow evaluation.
	class ShadowMapPass final
	{
	public:
		VULKAN_NON_COPIABLE(ShadowMapPass)

		/// @brief Shadow depth image resolution (square).
		static constexpr uint32_t kSize = 2048;

		/// @brief CPU mirror of the GLSL ShadowUBO block.
		struct ShadowUBO
		{
			glm::mat4 LightViewProj{ 1.0f };
		};

		/// @param device     Vulkan device — must outlive this object.
		/// @param imageCount Number of swapchain images (one UBO per frame).
		ShadowMapPass(const Vulkan::Device& device, uint32_t imageCount);

		~ShadowMapPass();

		/// @brief Compute the directional-light VP from the scene and upload it
		///        into the per-frame ShadowUBO at @p imageIndex.
		///        Must be called before Render() for the same imageIndex.
		void UpdateLightVP(uint32_t imageIndex, const Assets::Scene& scene);

		/// @brief Record the full shadow pass into @p commandBuffer.
		///        Performs depth image layout transitions around the render pass
		///        so the image is in SHADER_READ_ONLY_OPTIMAL when it returns.
		void Render(VkCommandBuffer commandBuffer,
					uint32_t        imageIndex,
					const Assets::Scene& scene);

		// ── Accessors consumed by GameRenderer descriptor bindings ────────────
		/// @brief VkImageView for the shadow depth image (binding 5 in main pass).
		VkImageView ShadowImageView() const;

		/// @brief Compare-enabled sampler (sampler2DShadow) for PCF (binding 5).
		VkSampler ShadowSampler() const;

		/// @brief Per-frame ShadowUBO buffer (binding 6 in main pass vert shader).
		const Vulkan::Buffer& LightVPBuffer(uint32_t imageIndex) const;

	private:
		void CreateDepthResources();
		void CreateRenderPass();
		void CreateFramebuffer();
		void CreateDescriptorSets(uint32_t imageCount);
		void CreatePipeline();

		/// @brief Compute a stable ortho light VP for the first directional light.
		///        Falls back to direction (0,−1,0.3) when no directional light exists.
		static ShadowUBO ComputeLightVP(const Assets::Scene& scene);

		/// @brief Emit a VkImageMemoryBarrier for the shadow depth image.
		void TransitionDepthImage(VkCommandBuffer commandBuffer,
								  VkImageLayout   oldLayout,
								  VkImageLayout   newLayout) const;

		// ── Non-owning reference ──────────────────────────────────────────────
		const Vulkan::Device& device_;

		// ── Depth image resources ─────────────────────────────────────────────
		std::unique_ptr<Vulkan::Image>        shadowImage_;
		std::unique_ptr<Vulkan::DeviceMemory> shadowMemory_;
		std::unique_ptr<Vulkan::ImageView>    shadowImageView_;
		std::unique_ptr<Vulkan::Sampler>      shadowSampler_;

		/// Tracks the current VkImageLayout so TransitionDepthImage can issue
		/// the correct barrier on every call.
		mutable VkImageLayout currentLayout_{ VK_IMAGE_LAYOUT_UNDEFINED };

		// ── Depth-only render pass + framebuffer ──────────────────────────────
		VkRenderPass  renderPass_ { VK_NULL_HANDLE };
		VkFramebuffer framebuffer_{ VK_NULL_HANDLE };

		// ── Shadow pipeline (vertex-only, depth bias enabled) ─────────────────
		VkPipelineLayout pipelineLayout_{ VK_NULL_HANDLE };
		VkPipeline       pipeline_      { VK_NULL_HANDLE };

		// ── Per-frame ShadowUBO resources ─────────────────────────────────────
		// One host-visible UBO per swapchain image; descriptors live in
		// descriptorSetManager_ at binding 0.
		std::unique_ptr<Vulkan::DescriptorSetManager>      descriptorSetManager_;
		std::vector<std::unique_ptr<Vulkan::Buffer>>       lightVPBuffers_;
		std::vector<Vulkan::DeviceMemory>                  lightVPMemories_;
	};

} // namespace Vulkan::Game
