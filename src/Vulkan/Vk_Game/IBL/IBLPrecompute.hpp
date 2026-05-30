#pragma once

#include "Vulkan/Vulkan.hpp"
#include <array>
#include <memory>
#include <vector>

namespace Vulkan
{
	class CommandPool;
	class Device;
	class DeviceMemory;
	class Image;
	class ImageView;
	class Sampler;
}

namespace Vulkan::Game
{
	// ─────────────────────────────────────────────────────────────────────────
	/// @brief Owns and pre-computes the three IBL textures required for
	///        physically-based Image-Based Lighting in the Game Renderer:
	///
	///   1. **Irradiance cubemap**  (32×32×6, RGBA16F)
	///      Hemisphere-integrated radiance — used for diffuse ambient.
	///
	///   2. **Prefiltered env cubemap** (128×128×6, RGBA16F, 7 mip levels)
	///      GGX-convolved environment at increasing roughness — used for
	///      specular highlights via the split-sum approximation.
	///
	///   3. **BRDF integration LUT** (512×512, RG16F)
	///      Pre-integrated F₀ scale (R) and bias (G) over (NdotV, roughness).
	///
	/// All three resources are computed by compute shaders dispatched in a
	/// single one-shot command buffer and are ready before the first frame.
	///
	/// The resulting image views and samplers are bound to descriptors 9/10/11
	/// in GameRenderer::CreateDescriptorSets().
	// ─────────────────────────────────────────────────────────────────────────
	class IBLPrecompute final
	{
	public:

		IBLPrecompute(const IBLPrecompute&)            = delete;
		IBLPrecompute& operator=(const IBLPrecompute&) = delete;

		/// @param device        Active Vulkan device.
		/// @param commandPool   A command pool on the graphics queue (used for
		///                      one-shot compute dispatches; waits idle before
		///                      the constructor returns).
		/// @param skyboxView    VkImageView of the scene skybox cubemap (source
		///                      environment for convolution).
		/// @param skyboxSampler VkSampler for the skybox cubemap.
		IBLPrecompute(const Vulkan::Device& device,
					  Vulkan::CommandPool&  commandPool,
					  VkImageView           skyboxView,
					  VkSampler             skyboxSampler);
		~IBLPrecompute();

		// ── Output accessors ──────────────────────────────────────────────────

		VkImageView IrradianceView()    const;
		VkSampler   IrradianceSampler() const;

		VkImageView PrefilteredView()    const;
		VkSampler   PrefilteredSampler() const;

		VkImageView BrdfLutView()    const;
		VkSampler   BrdfLutSampler() const;

		// ── Constants ─────────────────────────────────────────────────────────

		/// Number of roughness mip levels in the prefiltered cubemap.
		/// Mip 0 = roughness 0 (mirror), mip 6 = roughness 1 (fully diffuse).
		static constexpr uint32_t kPrefilteredMips = 7;

		static constexpr uint32_t kIrradianceSize  = 32;
		static constexpr uint32_t kPrefilteredSize = 128;
		static constexpr uint32_t kBrdfLutSize     = 512;

	private:

		// ── Creation helpers ──────────────────────────────────────────────────

		void CreateIrradianceResources();
		void CreatePrefilteredResources();
		void CreateBrdfLutResources();

		void CreateIrradiancePipeline();
		void CreatePrefilterPipeline();
		void CreateBrdfLutPipeline();

		void DispatchAll(Vulkan::CommandPool& commandPool);

		// Transition a VkImage from UNDEFINED → GENERAL (for imageStore writes)
		void TransitionToGeneral(VkCommandBuffer cmd, VkImage image,
								 uint32_t arrayLayers, uint32_t mipLevels) const;

		// Transition from GENERAL → SHADER_READ_ONLY_OPTIMAL (for sampling)
		void TransitionToShaderRead(VkCommandBuffer cmd, VkImage image,
									uint32_t arrayLayers, uint32_t mipLevels) const;

		// ── Device reference (non-owning) ─────────────────────────────────────
		const Vulkan::Device& device_;
		VkImageView           skyboxView_;
		VkSampler             skyboxSampler_;

		// ── Irradiance cubemap ────────────────────────────────────────────────
		std::unique_ptr<Vulkan::Image>       irradianceImage_;
		std::unique_ptr<Vulkan::DeviceMemory> irradianceMem_;
		std::unique_ptr<Vulkan::ImageView>   irradianceView_;     // whole-cube view
		std::unique_ptr<Vulkan::Sampler>     irradianceSampler_;
		std::array<VkImageView, 6>           irradianceFaceViews_{}; // per-face storage views

		// ── Prefiltered env cubemap ───────────────────────────────────────────
		std::unique_ptr<Vulkan::Image>       prefilteredImage_;
		std::unique_ptr<Vulkan::DeviceMemory> prefilteredMem_;
		std::unique_ptr<Vulkan::ImageView>   prefilteredView_;    // whole-cube + mips
		std::unique_ptr<Vulkan::Sampler>     prefilteredSampler_;
		// [mip][face] storage image views (compute writes one face/mip at a time)
		std::vector<std::array<VkImageView, 6>> prefilteredMipFaceViews_;

		// ── BRDF integration LUT (2D) ─────────────────────────────────────────
		std::unique_ptr<Vulkan::Image>       brdfLutImage_;
		std::unique_ptr<Vulkan::DeviceMemory> brdfLutMem_;
		std::unique_ptr<Vulkan::ImageView>   brdfLutView_;
		std::unique_ptr<Vulkan::Sampler>     brdfLutSampler_;

		// ── Compute pipeline: irradiance ──────────────────────────────────────
		VkDescriptorPool      irrDescPool_{ VK_NULL_HANDLE };
		VkDescriptorSetLayout irrDSL_{ VK_NULL_HANDLE };
		// One descriptor set per face (binding 1 changes per face)
		std::array<VkDescriptorSet, 6> irrSets_{};
		VkPipelineLayout      irrPipelineLayout_{ VK_NULL_HANDLE };
		VkPipeline            irrPipeline_{ VK_NULL_HANDLE };

		// ── Compute pipeline: specular prefilter ──────────────────────────────
		VkDescriptorPool      preDescPool_{ VK_NULL_HANDLE };
		VkDescriptorSetLayout preDSL_{ VK_NULL_HANDLE };
		// One descriptor set per (mip, face) pair
		std::vector<std::array<VkDescriptorSet, 6>> preSets_;
		VkPipelineLayout      prePipelineLayout_{ VK_NULL_HANDLE };
		VkPipeline            prePipeline_{ VK_NULL_HANDLE };

		// ── Compute pipeline: BRDF LUT ────────────────────────────────────────
		VkDescriptorPool      lutDescPool_{ VK_NULL_HANDLE };
		VkDescriptorSetLayout lutDSL_{ VK_NULL_HANDLE };
		VkDescriptorSet       lutSet_{ VK_NULL_HANDLE };
		VkPipelineLayout      lutPipelineLayout_{ VK_NULL_HANDLE };
		VkPipeline            lutPipeline_{ VK_NULL_HANDLE };
	};

} // namespace Vulkan::Game
