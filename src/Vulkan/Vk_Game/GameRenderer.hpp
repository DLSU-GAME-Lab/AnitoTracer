#pragma once

#include "Vulkan/Vulkan.hpp"
#include "Vulkan/Buffer.hpp"
#include "ShadowMapSettings.hpp"
#include "GameRendererMaterialProperties.hpp"
#include "From-GDGRAP2/EventBroadcaster.h"
#include <memory>
#include <vector>

namespace Vulkan::Game { class IBLPrecompute; }

namespace Assets
{
	class Scene;
	class UniformBuffer;
}

namespace Vulkan
{
	class CommandPool;
	class DepthBuffer;
	class DescriptorSetManager;
	class RenderPass;
	class SwapChain;
}

namespace Vulkan::Game
{
	class ShadowMapPass;
	class PointLightShadowPass;
}

namespace Vulkan::Game
{
	/// @brief Real-time rasterization renderer for the Game renderer mode.
	/// Renders the scene directly to the swapchain every frame using a
	/// standard forward-shading graphics pipeline.
	/// No ray tracing, no multi-frame accumulation.
	///
		/// Descriptor bindings (must match game_vert.spv / game_frag.spv):
		///   0 : Uniform buffer        (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		///   1 : Material buffer       (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		///   2 : Light buffer          (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		///   3 : Texture samplers[]    (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		///   4 : Skybox sampler        (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		///   5 : Shadow maps[4]        (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER — compare array)
		///   6 : ShadowUBO             (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER — frag only)
	class GameRenderer final : public Observer
	{
	public:

		VULKAN_NON_COPIABLE(GameRenderer)

		/// @brief Construct and initialize all Vulkan resources for this renderer.
		/// @param swapChain     The active swapchain — used for format, extent and image views.
		/// @param depthBuffer   The shared depth buffer owned by the Application.
		/// @param uniformBuffers Per-frame UBO array (one entry per swapchain image).
		/// @param scene         The loaded scene providing geometry + texture + light buffers.
		/// @param commandPool   A command pool on the graphics queue, used for one-shot
		///                      IBL pre-computation dispatches during construction.
		GameRenderer(
			const Vulkan::SwapChain& swapChain,
			const Vulkan::DepthBuffer& depthBuffer,
			const std::vector<Assets::UniformBuffer>& uniformBuffers,
			const Assets::Scene& scene,
			Vulkan::CommandPool& commandPool);

		~GameRenderer();

		/// @brief Record draw commands into commandBuffer for the given swapchain image.
		/// Called from RayTracer::Render() each frame.
		void Render(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		/// @brief Returns the descriptor set for the given swapchain image index.
		VkDescriptorSet DescriptorSet(uint32_t index) const;

		/// @brief Returns the pipeline layout (needed for push constants / descriptor binding).
		/// @brief Returns the raw VkPipelineLayout handle (needed for push constants).
		VkPipelineLayout PipelineLayoutHandle() const { return pipelineLayoutRaw_; }

		/// @brief Apply updated shadow settings immediately.
		///        Recreates only the ShadowMapPass and its dependent descriptor sets.
		///        Safe to call between frames (must NOT be called while the GPU
		///        is executing a frame that uses the old shadow resources).
		void ApplyShadowSettings(ShadowMapSettings settings);

		/// @brief Read-only access to the current shadow map settings.
		const ShadowMapSettings& GetShadowSettings() const;

		/// @brief Must be called once per frame BEFORE the command buffer begins
		///        recording (i.e. before Application::DrawFrame / commandBuffers_->Begin).
		///        Applies any shadow settings change that was queued via the
		///        ON_SHADOW_SETTINGS_CHANGED event during the previous frame's UI draw.
		void FlushPendingShadowReload();

/// @brief Returns a non-owning pointer to the IBL pre-compute resources,
/// or nullptr when the scene has no skybox and IBL was not computed.
const IBLPrecompute* GetIBLPrecompute() const { return iblPrecompute_.get(); }


	private:

		// Observer callback — responds to ON_SHADOW_SETTINGS_CHANGED.
		void onTriggeredEvent(std::string eventName,
							  std::shared_ptr<Parameters> parameters) override;

		/// Create the render pass: 1 color attachment (clear) + 1 depth attachment (clear).
		void CreateRenderPass();

		/// Create the descriptor pool + sets and write all buffer/image bindings.
		void CreateDescriptorSets(const std::vector<Assets::UniformBuffer>& uniformBuffers,
								  const Assets::Scene& scene);

		/// Create the graphics pipeline (vertex + fragment shaders, depth test, back-face cull).
		void CreatePipeline();

		/// Create one VkFramebuffer per swapchain image, attaching color + depth views.
		void CreateFramebuffers();

		/// Create the material properties buffer for normal mapping and other
		/// Game Renderer-specific material features.
		void CreateGameRendererMaterialPropsBuffer(const Assets::Scene& scene);

		// ── Owned Vulkan resources ──────────────────────────────────────────────
		std::unique_ptr<ShadowMapPass>           shadowMapPass_;
		std::unique_ptr<PointLightShadowPass>    pointLightShadowPass_;
		std::unique_ptr<IBLPrecompute>           iblPrecompute_;
		std::unique_ptr<Vulkan::RenderPass>           renderPass_;
		std::unique_ptr<Vulkan::DescriptorSetManager> descriptorSetManager_;

		// Material properties for normal mapping and PBR textures
		std::unique_ptr<Vulkan::Buffer> gameRendererMatPropsBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> gameRendererMatPropsBufferMemory_;

		// Managed manually so we can add push-constant ranges (the PipelineLayout
		// wrapper class does not expose that option).
		VkPipelineLayout pipelineLayoutRaw_{ VK_NULL_HANDLE };

		VULKAN_HANDLE(VkPipeline, pipeline_)

		std::vector<VkFramebuffer> framebuffers_;

		// ── Non-owning references ───────────────────────────────────────────────
		const Vulkan::SwapChain&                       swapChain_;
		const Vulkan::DepthBuffer&                     depthBuffer_;
		const Assets::Scene&                           scene_;
		const std::vector<Assets::UniformBuffer>*      uniformBuffers_{ nullptr };
		Vulkan::CommandPool*                           commandPool_{ nullptr };

		// ── Pending hot-reload ────────────────────────────────────────────────
		/// When set, the next Render() call will recreate shadowMapPass_ before drawing.
		bool                pendingShadowReload_{ false };
		ShadowMapSettings   pendingShadowSettings_{};
	};

} // namespace Vulkan::Game
