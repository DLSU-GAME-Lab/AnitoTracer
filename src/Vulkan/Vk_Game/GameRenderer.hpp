#pragma once

#include "Vulkan/Vulkan.hpp"
#include <memory>
#include <vector>

namespace Assets
{
	class Scene;
	class UniformBuffer;
}

namespace Vulkan
{
	class DepthBuffer;
	class DescriptorSetManager;
	class RenderPass;
	class SwapChain;
}

namespace Vulkan::Game
{
	/// @brief Real-time rasterization renderer for the Game renderer mode.
	/// Renders the scene directly to the swapchain every frame using a
	/// standard forward-shading graphics pipeline.
	/// No ray tracing, no multi-frame accumulation.
	///
	/// Descriptor bindings (must match game_vert.spv / game_frag.spv):
	///   0 : Uniform buffer       (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
	///   1 : Material buffer      (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
	///   2 : Light buffer         (VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
	///   3 : Texture samplers[]   (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
	///   4 : Skybox sampler       (VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
	class GameRenderer final
	{
	public:

		VULKAN_NON_COPIABLE(GameRenderer)

		/// @brief Construct and initialize all Vulkan resources for this renderer.
		/// @param swapChain    The active swapchain — used for format, extent and image views.
		/// @param depthBuffer  The shared depth buffer owned by the Application.
		/// @param uniformBuffers Per-frame UBO array (one entry per swapchain image).
		/// @param scene        The loaded scene providing geometry + texture + light buffers.
		GameRenderer(
			const Vulkan::SwapChain& swapChain,
			const Vulkan::DepthBuffer& depthBuffer,
			const std::vector<Assets::UniformBuffer>& uniformBuffers,
			const Assets::Scene& scene);

		~GameRenderer();

		/// @brief Record draw commands into commandBuffer for the given swapchain image.
		/// Called from RayTracer::Render() each frame.
		void Render(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		/// @brief Returns the descriptor set for the given swapchain image index.
		VkDescriptorSet DescriptorSet(uint32_t index) const;

		/// @brief Returns the pipeline layout (needed for push constants / descriptor binding).
		/// @brief Returns the raw VkPipelineLayout handle (needed for push constants).
		VkPipelineLayout PipelineLayoutHandle() const { return pipelineLayoutRaw_; }

	private:

		/// Create the render pass: 1 color attachment (clear) + 1 depth attachment (clear).
		void CreateRenderPass();

		/// Create the descriptor pool + sets and write all buffer/image bindings.
		void CreateDescriptorSets(const std::vector<Assets::UniformBuffer>& uniformBuffers,
								  const Assets::Scene& scene);

		/// Create the graphics pipeline (vertex + fragment shaders, depth test, back-face cull).
		void CreatePipeline();

		/// Create one VkFramebuffer per swapchain image, attaching color + depth views.
		void CreateFramebuffers();

		// ── Owned Vulkan resources ──────────────────────────────────────────────
		std::unique_ptr<Vulkan::RenderPass>           renderPass_;
		std::unique_ptr<Vulkan::DescriptorSetManager> descriptorSetManager_;

		// Managed manually so we can add push-constant ranges (the PipelineLayout
		// wrapper class does not expose that option).
		VkPipelineLayout pipelineLayoutRaw_{ VK_NULL_HANDLE };

		VULKAN_HANDLE(VkPipeline, pipeline_)

		std::vector<VkFramebuffer> framebuffers_;

		// ── Non-owning references ───────────────────────────────────────────────
		const Vulkan::SwapChain&   swapChain_;
		const Vulkan::DepthBuffer& depthBuffer_;
		const Assets::Scene&       scene_;
	};

} // namespace Vulkan::Game
