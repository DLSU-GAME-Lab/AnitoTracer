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
	class PipelineLayout;
	class RenderPass;
	class SwapChain;

	class ObjectPickingPipeline final
	{
	public:
		VULKAN_NON_COPIABLE(ObjectPickingPipeline)

		ObjectPickingPipeline(
			const SwapChain& swapChain,
			const DepthBuffer& depthBuffer,
			const std::vector<Assets::UniformBuffer>& uniformBuffers,
			const Assets::Scene& scene);
		~ObjectPickingPipeline();

		VkDescriptorSet DescriptorSet(uint32_t index) const;
		const class PipelineLayout& PipelineLayout() const { return *pipelineLayout_; }
		const class RenderPass& RenderPass() const { return *renderPass_; }

	private:

		const SwapChain& swapChain_;

		VULKAN_HANDLE(VkPipeline, pipeline_)

		std::unique_ptr<class DescriptorSetManager> descriptorSetManager_;
		std::unique_ptr<class PipelineLayout> pipelineLayout_;
		std::unique_ptr<class RenderPass> renderPass_;
	};

}
