#pragma once

#include "Vulkan.hpp"
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
	class PipelineLayout;
	class RenderPass;
	class SwapChain;

	class GraphicsPipeline final
	{
	public:

		VULKAN_NON_COPIABLE(GraphicsPipeline)

		GraphicsPipeline(
			const SwapChain& swapChain, 
			const DepthBuffer& depthBuffer,
			const std::vector<Assets::UniformBuffer>& uniformBuffers,
			const Assets::Scene& scene,
			bool isWireFrame);
		~GraphicsPipeline();

		VkDescriptorSet DescriptorSet(uint32_t index) const;
		bool IsWireFrame() const { return isWireFrame_; }
		const class PipelineLayout& PipelineLayout() const { return *pipelineLayout_; }
		const class RenderPass& RenderPass() const { return *renderPass_; }

		VkPipeline SkyboxPipeline() const { return skyboxPipeline_; }
		const class PipelineLayout& SkyboxPipelineLayout() const { return *skyboxPipelineLayout_; }
		VkDescriptorSet SkyboxDescriptorSet(uint32_t index) const;

	private:

		const SwapChain& swapChain_;
		const bool isWireFrame_;

		VULKAN_HANDLE(VkPipeline, pipeline_)

		std::unique_ptr<class DescriptorSetManager> descriptorSetManager_;
		std::unique_ptr<class PipelineLayout> pipelineLayout_;
		std::unique_ptr<class RenderPass> renderPass_;

		VkPipeline skyboxPipeline_ = VK_NULL_HANDLE;
		std::unique_ptr<class DescriptorSetManager> skyboxDescriptorSetManager_;
		std::unique_ptr<class PipelineLayout> skyboxPipelineLayout_;

		
	};

}
