#pragma once
#include "Vulkan/Vulkan.hpp"
#include <memory>

namespace Vulkan
{
	class DescriptorSetManager;
	class SwapChain;
	class ImageView;
}

class ComputePipelineLayout;

class ComputePipeline final
{
public:

	VULKAN_NON_COPIABLE(ComputePipeline)

	ComputePipeline(const Vulkan::SwapChain& swapChain,
		const VkBuffer pixelSamplesBuffer,
		const VkBuffer workQueueBuffer,
		const VkBuffer workQueueCountBuffer);
	~ComputePipeline();

	const class ComputePipelineLayout& PipelineLayout() const { return *pipelineLayout_; }

private:

	const Vulkan::SwapChain& swapChain_;

	VULKAN_HANDLE(VkPipeline, pipeline_)

	std::unique_ptr<Vulkan::DescriptorSetManager> descriptorSetManager_;
	std::unique_ptr<class ComputePipelineLayout> pipelineLayout_;
};