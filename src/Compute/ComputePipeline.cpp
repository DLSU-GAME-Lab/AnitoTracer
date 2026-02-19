#include "ComputePipeline.hpp"
#include "ComputePipelineLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/DescriptorBinding.hpp"
#include "Vulkan/DescriptorSetManager.hpp"
#include "Vulkan/DescriptorSets.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/Vulkan.hpp"
#include <From-GDGRAP2/Debug.h>

ComputePipeline::ComputePipeline(const Vulkan::Device& device, uint32_t framesInFlight) : device_(device), framesInFlight_(framesInFlight)
{
	const std::vector<Vulkan::DescriptorBinding> descriptorBindings =
	{
		// Binding 0
		{0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 1
		{1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 2
		{ 2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
	};

	descriptorSetManager_.reset(new Vulkan::DescriptorSetManager(device, descriptorBindings, framesInFlight));
	pipelineLayout_.reset(new class ComputePipelineLayout(device, descriptorSetManager_->DescriptorSetLayout()));
}

ComputePipeline::~ComputePipeline()
{
	for (VkPipeline& pipeline : pipelines_)
	{
		if (pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(device_.Handle(), pipeline, nullptr);
			pipeline = VK_NULL_HANDLE;
		}
	}
	
	pipelines_.clear();
	pipelineLayout_.reset();
	descriptorSetManager_.reset();
}

VkDescriptorSet ComputePipeline::DescriptorSet(const uint32_t index) const
{
	return descriptorSetManager_->DescriptorSets().Handle(index);
}

size_t ComputePipeline::CreatePipeline(const std::string& filename)
{
	const Vulkan::ShaderModule computeShader(device_, filename);

	VkComputePipelineCreateInfo  pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = pipelineLayout_->Handle();
	pipelineInfo.stage = computeShader.CreateShaderStage(VK_SHADER_STAGE_COMPUTE_BIT);

	VkPipeline pipeline = VK_NULL_HANDLE;

	Vulkan::Check(
		vkCreateComputePipelines(device_.Handle(), nullptr, 1, &pipelineInfo, nullptr, &pipeline),
		"create compute pipeline"
	);

	pipelines_.push_back(pipeline);

	return pipelines_.size() - 1;
}

void ComputePipeline::UpdateDescriptorSet(const std::vector<Vulkan::Buffer*>& buffers, uint32_t frameIndex)
{
	if(buffers.size() > 3)
	{
		Debug::Log("Too many buffers provided to update compute pipeline descriptor set. Max is 3.");
	}

	auto& descriptorSets = descriptorSetManager_->DescriptorSets();

	std::vector<VkDescriptorBufferInfo> bufferInfos;
	bufferInfos.reserve(buffers.size());

	std::vector<VkWriteDescriptorSet> descriptorWrites;
	descriptorWrites.reserve(buffers.size());

	for (uint32_t j = 0; j < buffers.size(); j++)
	{
		VkDescriptorBufferInfo info{};
		info.buffer = buffers[j]->Handle();
		info.offset = 0;
		info.range = VK_WHOLE_SIZE;

		bufferInfos.push_back(info);

		VkWriteDescriptorSet write = descriptorSets.Bind(frameIndex, j, bufferInfos.back());
		descriptorWrites.push_back(write);
	}

	descriptorSets.UpdateDescriptors(frameIndex, descriptorWrites);

}
