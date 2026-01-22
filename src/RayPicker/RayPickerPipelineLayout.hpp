#pragma once
#include "Vulkan/Vulkan.hpp"
#include "Vulkan/Device.hpp"

namespace Vulkan
{
	class DescriptorSetLayout;
}

class RayPickerPipelineLayout final
{
public:

	VULKAN_NON_COPIABLE(RayPickerPipelineLayout)

	RayPickerPipelineLayout(const Vulkan::Device& device, const Vulkan::DescriptorSetLayout& descriptorSetLayout);
	~RayPickerPipelineLayout();

private:
	const Vulkan::Device& device_; 

	VULKAN_HANDLE(VkPipelineLayout, pipelineLayout_)
};