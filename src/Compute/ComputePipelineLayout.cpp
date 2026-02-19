#include "ComputePipelineLayout.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"

ComputePipelineLayout::ComputePipelineLayout(const Vulkan::Device& device, const Vulkan::DescriptorSetLayout& descriptorSetLayout) : device_(device)
{
	VkDescriptorSetLayout descriptorSetLayouts[] = { descriptorSetLayout.Handle() };

	struct GenericPC
	{
		uint32_t width;
		uint32_t height;
		float probability;
		uint32_t frameSeed;
	};

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(GenericPC);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	Vulkan::Check(vkCreatePipelineLayout(device_.Handle(), &pipelineLayoutInfo, nullptr, &pipelineLayout_),
		"create pipeline layout");
}

ComputePipelineLayout::~ComputePipelineLayout()
{
	if (pipelineLayout_ != nullptr)
	{
		vkDestroyPipelineLayout(device_.Handle(), pipelineLayout_, nullptr);
		pipelineLayout_ = nullptr;
	}
}