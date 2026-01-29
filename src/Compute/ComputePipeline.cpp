#include "ComputePipeline.hpp"
#include "ComputePipelineLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/DescriptorBinding.hpp"
#include "Vulkan/DescriptorSetManager.hpp"
#include "Vulkan/DescriptorSets.hpp"
#include "Vulkan/ShaderModule.hpp"
#include <Utilities/FileUtils.h>

ComputePipeline::ComputePipeline(
	const Vulkan::SwapChain& swapChain, 
	const VkBuffer pixelSamplesBuffer, 
	const VkBuffer workQueueBuffer, 
	const VkBuffer workQueueCountBuffer ) : swapChain_(swapChain)
{
	const auto& device = swapChain.Device();

	const std::vector<Vulkan::DescriptorBinding> descriptorBindings =
	{
		// Pixel Samples Buffer
		{0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Work Queue Buffer
		{1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Work Queue Count Buffer
		{2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
	};

	descriptorSetManager_.reset(new Vulkan::DescriptorSetManager(device, descriptorBindings, swapChain.ImageViews().size()));
	auto& descriptorSets = descriptorSetManager_->DescriptorSets();

	for (uint32_t i = 0; i != swapChain.Images().size(); ++i)
	{
		// Pixel Samples Buffer
		VkDescriptorBufferInfo pixelSamplesBufferInfo = {};
		pixelSamplesBufferInfo.buffer = pixelSamplesBuffer;
		pixelSamplesBufferInfo.range = VK_WHOLE_SIZE;

		// Work Queue Buffer
		VkDescriptorBufferInfo workQueueBufferInfo = {};
		workQueueBufferInfo.buffer = workQueueBuffer;
		workQueueBufferInfo.range = VK_WHOLE_SIZE;

		// Work Queue Count Buffer
		VkDescriptorBufferInfo workQueueCountBufferInfo = {};
		workQueueCountBufferInfo.buffer = workQueueCountBuffer;
		workQueueCountBufferInfo.range = VK_WHOLE_SIZE;

		std::vector<VkWriteDescriptorSet> descriptorWrites =
		{
			descriptorSets.Bind(i, 0, pixelSamplesBufferInfo),
			descriptorSets.Bind(i, 1, workQueueBufferInfo),
			descriptorSets.Bind(i, 2, workQueueCountBufferInfo),
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

ComputePipeline::~ComputePipeline()
{
	if (pipeline_ != nullptr)
	{
		vkDestroyPipeline(swapChain_.Device().Handle(), pipeline_, nullptr);
		pipeline_ = nullptr;
	}

	pipelineLayout_.reset();
	descriptorSetManager_.reset();
}
