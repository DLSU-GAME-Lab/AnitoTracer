#pragma once

#include "UI/AUIScreen.h"
#include "FrameBuffer.hpp"
#include "WindowConfig.hpp"
#include <vector>
#include <memory>

#include "Image.hpp"
#include "Assets/UniformBuffer.hpp"

namespace Assets
{
	class Scene;
	class UniformBufferObject;
	class PushConstantModel;
	class UniformBuffer;
	class Model;
}

namespace Vulkan
{
	class SwapChain;
	class DeviceMemory;
	class Sampler;
	class Viewport : public AUIScreen
	{
	public:
		VULKAN_NON_COPIABLE(Viewport)

		virtual ~Viewport();

		Viewport(const class SwapChain& swapChain, const class ::Assets::Scene& scene);

		const class SwapChain& SwapChain() const { return swapChain_; }

		/*const class DepthBuffer& DepthBuffer() const { return *depthBuffer_; }
		const std::vector<Assets::UniformBuffer>& UniformBuffers() const { return uniformBuffers_; }
		const class GraphicsPipeline& GraphicsPipeline() const { return *graphicsPipeline_; }
		const class FrameBuffer& FrameBuffer(const size_t i) const { return frameBuffers_[i]; }
		*/

		virtual Assets::UniformBufferObject GetUniformBufferObject(VkExtent2D extent) const/* = 0*/;
		virtual Assets::PushConstantModel GetPushConstantModel(const Assets::Model& model) const/* = 0*/;

		virtual void RenderRasterized(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		virtual void RenderRayTraced(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkImage image);
		virtual void drawUI() override;

		/*
		virtual void OnKey(int key, int scancode, int action, int mods) {}
		virtual void OnCursorPosition(double xpos, double ypos) {}
		virtual void OnMouseButton(int button, int action, int mods) {}
		virtual void OnScroll(double xoffset, double yoffset) {}
		*/

		bool isWireFrame_{};

	private:
		void TransitionImageLayout(CommandPool& commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layerCount = 1);
		DeviceMemory AllocateImageMemory(VkImage image) const;

		void UpdateUniformBuffer(uint32_t imageIndex);

		const class SwapChain& swapChain_;
		const class Assets::Scene& scene_;

		std::vector<Assets::UniformBuffer> uniformBuffers_;
		std::unique_ptr<class DepthBuffer> depthBuffer_;
		std::unique_ptr<class GraphicsPipeline> graphicsPipeline_;
		std::vector<class FrameBuffer> frameBuffers_;
		std::unique_ptr<class CommandPool> commandPool_;
		std::unique_ptr<class CommandBuffers> commandBuffers_;
		std::vector<class Semaphore> imageAvailableSemaphores_;
		std::vector<class Semaphore> renderFinishedSemaphores_;
		std::vector<class Fence> inFlightFences_;

		std::shared_ptr<class RenderPass> renderPass_;

		std::unique_ptr<Image> outputImage_;
		std::unique_ptr<DeviceMemory> outputImageMemory_;
		std::unique_ptr<ImageView> outputImageView_;
		VkImage rtImage_;
		VkDescriptorSet rtDset_;

		std::unique_ptr<Vulkan::Sampler> sampler_;

		VkDescriptorSet viewportDSet_;

		size_t currentFrame_{};
	};
}