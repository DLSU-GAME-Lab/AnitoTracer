#pragma once

#include "Vulkan/Vulkan.hpp"
#include "Utilities/Glm.hpp"  // MUST come before any other glm include
#include "ShadowMapSettings.hpp"
#include <array>
#include <memory>
#include <vector>
#include <optional>

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
	/// Maximum number of point lights that can cast shadows simultaneously.
	inline constexpr uint32_t kMaxPointShadowLights = 4;

	/// @brief Cubemap depth shadow pass for point lights.
	///
	/// Creates and owns (per point light):
	///  - A cubemap VK_FORMAT_D32_SFLOAT depth image (6 faces) + framebuffers (one per face).
	/// And globally:
	///  - One shared compare-enabled cubemap sampler (samplerCubeShadow, LESS_OR_EQUAL).
	///  - A depth-only VkRenderPass + graphics pipeline.
	///  - Per-frame PointShadowUBO containing light positions and view-projection 
	///    matrices for each of the 6 cubemap faces.
	///
	/// Usage per frame (called from GameRenderer::Render):
	///   1. pointShadowPass_->UpdateLightVP(imageIndex, scene_)
	///        — collects all point lights, fills per-frame UBO with cubemap VP matrices.
	///   2. pointShadowPass_->Render(cmd, imageIndex, scene_)
	///        — records 6 depth sub-passes per active point light (one per cubemap face).
	///      The main pass reads PointShadowImageViews() / PointShadowSampler() (binding 7,
	///      array of kMaxPointShadowLights cubemaps) and PointLightVPBuffer() (binding 8).
	class PointLightShadowPass final
	{
	public:
		VULKAN_NON_COPIABLE(PointLightShadowPass)

		/// Maximum number of point lights that can cast shadows simultaneously.
		static constexpr uint32_t kMaxPointShadowLights = Vulkan::Game::kMaxPointShadowLights;

		/// @brief CPU mirror of the GLSL PointShadowUBO block (std140 compatible).
		/// Stores cubemap VP matrices: 6 views per light (one per face).
		struct PointShadowUBO
		{
			/// For each active light: the 6 cubemap face view matrices (perspective).
			/// Layout: [light 0 face 0..5], [light 1 face 0..5], ... (up to kMaxPointShadowLights)
			glm::mat4 CubemapViewProj[kMaxPointShadowLights * 6]{};

			/// Light positions (world-space, for depth calculation in shader).
			glm::vec4 LightPositions[kMaxPointShadowLights]{};

			/// Number of active point lights (< kMaxPointShadowLights).
				uint32_t Count{ 0 };
				/// Far plane used when building cubemap VP matrices — stored here so
				/// both the shadow fragment shader and the main fragment shader can
				/// read the same value for linear-depth encoding / comparison.
				float FarPlane{ 1000.0f };
				float _pad[2]{ 0.f, 0.f }; // align to 16 bytes
		};

		/// Optional per-point-light-slot overrides for shadow settings.
		struct PointShadowLightSettings
		{
			std::optional<uint32_t> Resolution;         // per-face resolution (square)
			std::optional<bool>     DepthBiasEnable;
			std::optional<float>    DepthBiasConstantFactor;
			std::optional<float>    DepthBiasSlopeFactor;
			std::optional<float>    DepthBiasClamp;
			std::optional<float>    NearPlane;
			std::optional<float>    FarPlane;           // far plane for perspective view
		};

		/// Settings for point light shadow rendering.
		struct PointShadowSettings final
		{
			// ── Per-face depth image ───────────────────────────────────────────
			uint32_t Resolution{ 512 };                 // per-face resolution (default 512²)
			VkFormat DepthFormat{ VK_FORMAT_D32_SFLOAT };

			// ── Sampler (shared) ───────────────────────────────────────────────
				VkFilter MagFilter{ VK_FILTER_LINEAR };
				VkFilter MinFilter{ VK_FILTER_LINEAR };
				VkSamplerAddressMode AddressMode{ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE };
				VkBorderColor BorderColor{ VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE };
				VkCompareOp CompareOp{ VK_COMPARE_OP_LESS };
				VkSamplerMipmapMode MipmapMode{ VK_SAMPLER_MIPMAP_MODE_NEAREST };

			// ── Depth bias ─────────────────────────────────────────────────────
			bool DepthBiasEnable{ true };
			float DepthBiasConstantFactor{ 0.75f };     // lower than directional (closer to camera)
			float DepthBiasSlopeFactor{ 1.5f };
			float DepthBiasClamp{ 0.0f };

			// ── Perspective frustum ────────────────────────────────────────────
			float NearPlane{ 0.1f };
			float FarPlane{ 1000.0f };                  // large far plane for large scenes

			// ── Rasteriser ─────────────────────────────────────────────────────
			VkCullModeFlags CullMode{ VK_CULL_MODE_BACK_BIT };

			// ── Per-light overrides ────────────────────────────────────────────
			std::vector<PointShadowLightSettings> LightOverrides;
		};

		/// @param device     Vulkan device — must outlive this object.
		/// @param imageCount Number of swapchain images (one UBO per frame).
		/// @param settings   Point shadow properties; sensible defaults provided.
		PointLightShadowPass(const Vulkan::Device& device,
							 uint32_t              imageCount,
							 PointShadowSettings   settings = {});

		~PointLightShadowPass();

		/// @brief Collect all point lights from @p scene, compute cubemap VP matrices,
		///        and upload into the per-frame PointShadowUBO.
		///        Must be called before Render() for the same imageIndex.
		void UpdateLightVP(uint32_t imageIndex, const Assets::Scene& scene);

		/// @brief Record all cubemap shadow sub-passes into @p commandBuffer — 6 faces
		///        per active point light, one depth-only render pass per face.
		///        Every shadow cubemap is left in SHADER_READ_ONLY_OPTIMAL when done.
		void Render(VkCommandBuffer commandBuffer,
					uint32_t        imageIndex,
					const Assets::Scene& scene);

		// ── Accessors consumed by GameRenderer descriptor bindings ────────────

		/// @brief Returns VkImageViews for all kMaxPointShadowLights point shadow cubemaps
		///        (binding 7 in main pass, descriptorCount = kMaxPointShadowLights).
		std::vector<VkImageView> PointShadowImageViews() const;

		/// @brief Shared compare-enabled cubemap sampler (samplerCubeShadow) for PCF.
		VkSampler PointShadowSampler() const;

		/// @brief Per-frame PointShadowUBO buffer (binding 8 in main pass, fragment).
		const Vulkan::Buffer& PointLightVPBuffer(uint32_t imageIndex) const;

		/// @brief Read-only access to the active point shadow settings.
		const PointShadowSettings& Settings() const { return settings_; }

	private:
		/// @brief Fully-resolved settings for a single point-light-slot shadow.
		struct ResolvedLightSettings
		{
			uint32_t Resolution;
			bool     DepthBiasEnable;
			float    DepthBiasConstantFactor;
			float    DepthBiasSlopeFactor;
			float    DepthBiasClamp;
			float    NearPlane;
			float    FarPlane;
		};

		/// @brief Internal per-point-light cubemap resources.
		struct PointShadowLayer
		{
			std::unique_ptr<Vulkan::Image>        CubemapImage;
			std::unique_ptr<Vulkan::DeviceMemory> Memory;
			std::unique_ptr<Vulkan::ImageView>    CubemapView;

			// One view per cubemap face (kept alive for framebuffers) + one framebuffer per face
			std::array<VkImageView, 6>       FaceViews{};
			std::array<VkFramebuffer, 6>     Framebuffers{};
			mutable VkImageLayout            CurrentLayout{ VK_IMAGE_LAYOUT_UNDEFINED };

			uint32_t                         Resolution{ 512 };
		};

		ResolvedLightSettings ResolveLightSettings(uint32_t lightIndex) const;

		void CreateDepthResources();
		void CreateRenderPass();
		void CreateFramebuffers();
		void CreateDescriptorSets(uint32_t imageCount);
		void CreatePipeline();

		/// Compute 6 perspective VP matrices for a cubemap centered at lightPos.
		void ComputeCubemapVP(const glm::vec3& lightPos,
							  const ResolvedLightSettings& rs,
							  std::array<glm::mat4, 6>& outVP) const;

		void TransitionCubemapImage(VkCommandBuffer commandBuffer,
								   PointShadowLayer& layer,
								   VkImageLayout newLayout) const;

		// ── Non-owning reference ──────────────────────────────────────────────
		const Vulkan::Device& device_;

		// ── Settings ──────────────────────────────────────────────────────────
		PointShadowSettings settings_;

		// ── Per-light cubemap resources ───────────────────────────────────────
		std::vector<PointShadowLayer> pointLayers_;

		// Shared cubemap compare sampler
		std::unique_ptr<Vulkan::Sampler> pointShadowSampler_;

		// Cached count of active point lights
		uint32_t activePointLightCount_{ 0 };

		// ── Depth-only render pass (shared) ─────────────────────────────────────
		VkRenderPass renderPass_{ VK_NULL_HANDLE };

		// ── Shadow pipeline (vertex-only, depth bias enabled) ─────────────────
		VkPipelineLayout pipelineLayout_{ VK_NULL_HANDLE };
		VkPipeline       pipeline_      { VK_NULL_HANDLE };

		// ── Per-frame PointShadowUBO resources ──────────────────────────────────
		std::unique_ptr<Vulkan::DescriptorSetManager>      descriptorSetManager_;
		std::vector<std::unique_ptr<Vulkan::Buffer>>       pointLightVPBuffers_;
		std::vector<Vulkan::DeviceMemory>                  pointLightVPMemories_;
	};

} // namespace Vulkan::Game
