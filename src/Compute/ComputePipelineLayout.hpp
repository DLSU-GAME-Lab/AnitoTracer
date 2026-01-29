#pragma once
#include "Vulkan/Vulkan.hpp"

namespace Vulkan
{
	class DescriptorSetLayout;
	class Device;
}

class ComputePipelineLayout final
{
public:

	VULKAN_NON_COPIABLE(ComputePipelineLayout)

	ComputePipelineLayout(const Vulkan::Device& device, const Vulkan::DescriptorSetLayout& descriptorSetLayout);
	~ComputePipelineLayout();

private:
	const Vulkan::Device& device_;

	VULKAN_HANDLE(VkPipelineLayout, pipelineLayout_)
};
