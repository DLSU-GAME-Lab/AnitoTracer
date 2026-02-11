#pragma once
#include "Vulkan/Vulkan.hpp"
#include <memory>

namespace Vulkan
{
	class DescriptorSetManager;
	class Device;
}

class ComputePipelineLayout;

class WorkLoaderPipeline final
{
public:

	VULKAN_NON_COPIABLE(WorkLoaderPipeline)

	WorkLoaderPipeline(const Vulkan::Device& device, const VkBuffer workQueueBuffer, const VkBuffer workCountBuffer, uint32_t framesInFlight);
	~WorkLoaderPipeline();

	const class ComputePipelineLayout& PipelineLayout() const { return *pipelineLayout_; }
	VkDescriptorSet DescriptorSet(const uint32_t index) const;

private:

	const Vulkan::Device& device_;

	VULKAN_HANDLE(VkPipeline, pipeline_)

	std::unique_ptr<Vulkan::DescriptorSetManager> descriptorSetManager_;
	std::unique_ptr<class ComputePipelineLayout> pipelineLayout_;
};