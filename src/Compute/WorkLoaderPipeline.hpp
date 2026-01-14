#pragma once
#include "Vulkan/Vulkan.hpp"
#include <memory>

namespace Vulkan
{
	class DescriptorSetManager;
	class SwapChain;
}

class ComputePipelineLayout;

class WorkLoaderPipeline final
{
public:

	VULKAN_NON_COPIABLE(WorkLoaderPipeline)

	WorkLoaderPipeline(const Vulkan::SwapChain& swapChain, const VkBuffer pixelWeightsBuffer,
			const VkBuffer workQueueBuffer, const VkBuffer workCountBuffer, const VkBuffer rayCountBuffer);
	~WorkLoaderPipeline();

	const class ComputePipelineLayout& PipelineLayout() const { return *pipelineLayout_; }
	VkDescriptorSet DescriptorSet(const uint32_t index) const;

private:

	const Vulkan::SwapChain& swapChain_;

	VULKAN_HANDLE(VkPipeline, pipeline_)

		std::unique_ptr<Vulkan::DescriptorSetManager> descriptorSetManager_;
	std::unique_ptr<class ComputePipelineLayout> pipelineLayout_;
};