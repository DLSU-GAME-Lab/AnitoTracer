#include "WorkLoaderPipeline.hpp"
#include "ComputePipelineLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/DescriptorBinding.hpp"
#include "Vulkan/DescriptorSetManager.hpp"
#include "Vulkan/DescriptorSets.hpp"
#include "Vulkan/ShaderModule.hpp"
#include <Utilities/FileUtils.h>

WorkLoaderPipeline::WorkLoaderPipeline(const Vulkan::Device& device, const VkBuffer workQueueBuffer, const VkBuffer workCountBuffer, uint32_t framesInFlight) : device_(device)
{
	const std::vector<Vulkan::DescriptorBinding> descriptorBindings =
	{
		// Work Queue Buffer
		{0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Work Count Buffer
		{1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
	};

	descriptorSetManager_.reset(new Vulkan::DescriptorSetManager(device, descriptorBindings, framesInFlight));
	auto& descriptorSets = descriptorSetManager_->DescriptorSets();

	for (uint32_t i = 0; i != framesInFlight; ++i)
	{
		// Work Queue Buffer
		VkDescriptorBufferInfo WQBInfo = {};
		WQBInfo.buffer = workQueueBuffer;
		WQBInfo.range = VK_WHOLE_SIZE;

		// Work Count Buffer
		VkDescriptorBufferInfo WCBInfo = {};
		WCBInfo.buffer = workCountBuffer;
		WCBInfo.range = VK_WHOLE_SIZE;

		std::vector<VkWriteDescriptorSet> descriptorWrites =
		{
			descriptorSets.Bind(i, 0, WQBInfo),
			descriptorSets.Bind(i, 1, WCBInfo),
		};

		descriptorSets.UpdateDescriptors(i, descriptorWrites);
	}

	pipelineLayout_.reset(new class ComputePipelineLayout(device, descriptorSetManager_->DescriptorSetLayout()));

	const Vulkan::ShaderModule computeShader(device, FileUtils::getAssetsFolderPath().generic_string() + "/shaders/WorkLoader.comp.spv");

	VkComputePipelineCreateInfo  pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = pipelineLayout_->Handle();
	pipelineInfo.stage = computeShader.CreateShaderStage(VK_SHADER_STAGE_COMPUTE_BIT);

	Vulkan::Check(vkCreateComputePipelines(device.Handle(), nullptr, 1, &pipelineInfo, nullptr, &pipeline_), "create compute pipeline");
}

WorkLoaderPipeline::~WorkLoaderPipeline()
{
	if (pipeline_ != VK_NULL_HANDLE)
	{
		vkDestroyPipeline(device_.Handle(), pipeline_, nullptr);
		pipeline_ = VK_NULL_HANDLE;
	}

	pipelineLayout_.reset();
	descriptorSetManager_.reset();
}

VkDescriptorSet WorkLoaderPipeline::DescriptorSet(const uint32_t index) const
{
	return descriptorSetManager_->DescriptorSets().Handle(index);
}
