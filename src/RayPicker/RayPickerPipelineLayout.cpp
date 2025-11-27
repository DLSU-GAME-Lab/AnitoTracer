#include "RayPickerPipelineLayout.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "RayPickerUBO.hpp"

RayPickerPipelineLayout::RayPickerPipelineLayout(const Vulkan::Device& device, const Vulkan::DescriptorSetLayout& descriptorSetLayout) : device_(device)
{
	VkDescriptorSetLayout descriptorSetLayouts[] = { descriptorSetLayout.Handle() };

	VkPushConstantRange range = {};
	range.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	range.offset = 0;
	range.size = sizeof(PushConstantScreenPosition);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = 1; 
	pipelineLayoutInfo.pPushConstantRanges = &range; 

	Vulkan::Check(vkCreatePipelineLayout(device_.Handle(), &pipelineLayoutInfo, nullptr, &pipelineLayout_),
		"create pipeline layout");
}

RayPickerPipelineLayout::~RayPickerPipelineLayout()
{
	if (pipelineLayout_ != nullptr)
	{
		vkDestroyPipelineLayout(device_.Handle(), pipelineLayout_, nullptr);
		pipelineLayout_ = nullptr;
	}
}