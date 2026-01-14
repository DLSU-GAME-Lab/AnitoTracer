#include "WorkLoaderPipeline.hpp"
#include "ComputePipelineLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/DescriptorBinding.hpp"
#include "Vulkan/DescriptorSetManager.hpp"
#include "Vulkan/DescriptorSets.hpp"
#include "Vulkan/ShaderModule.hpp"
#include <Utilities/FileUtils.h>

WorkLoaderPipeline::WorkLoaderPipeline(
	const Vulkan::SwapChain& swapChain,
	const VkBuffer pixelWeightsBuffer,
	const VkBuffer workQueueBuffer,
	const VkBuffer workCountBuffer,
	const VkBuffer rayCountBuffer) : swapChain_(swapChain)
{
	const auto& device = swapChain.Device();

	const std::vector<Vulkan::DescriptorBinding> descriptorBindings =
	{
		// Pixel Weights Buffer
		{0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Work Queue Buffer
		{1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Work Count Buffer
		{2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Ray Count Buffer
		{3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
	};

	descriptorSetManager_.reset(new Vulkan::DescriptorSetManager(device, descriptorBindings, swapChain.ImageViews().size()));
	auto& descriptorSets = descriptorSetManager_->DescriptorSets();

	for (uint32_t i = 0; i != swapChain.Images().size(); ++i)
	{
		// Pixel Weights Buffer
		VkDescriptorBufferInfo PWBInfo = {};
		PWBInfo.buffer = pixelWeightsBuffer;
		PWBInfo.range = VK_WHOLE_SIZE;

		// Work Queue Buffer
		VkDescriptorBufferInfo WQBInfo = {};
		WQBInfo.buffer = workQueueBuffer;
		WQBInfo.range = VK_WHOLE_SIZE;

		// Work Count Buffer
		VkDescriptorBufferInfo WCBInfo = {};
		WCBInfo.buffer = workCountBuffer;
		WCBInfo.range = VK_WHOLE_SIZE;

		// Ray Count Buffer
		VkDescriptorBufferInfo RCBInfo = {};
		RCBInfo.buffer = rayCountBuffer;
		RCBInfo.range = VK_WHOLE_SIZE;

		std::vector<VkWriteDescriptorSet> descriptorWrites =
		{
			descriptorSets.Bind(i, 0, PWBInfo),
			descriptorSets.Bind(i, 1, WQBInfo),
			descriptorSets.Bind(i, 2, RCBInfo),
			descriptorSets.Bind(i, 3, PWBInfo),
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
	if (pipeline_ != nullptr)
	{
		vkDestroyPipeline(swapChain_.Device().Handle(), pipeline_, nullptr);
		pipeline_ = nullptr;
	}

	pipelineLayout_.reset();
	descriptorSetManager_.reset();
}

VkDescriptorSet WorkLoaderPipeline::DescriptorSet(const uint32_t index) const
{
	return descriptorSetManager_->DescriptorSets().Handle(index);
}
