#include "PixelManagementPipeline.hpp"
#include "ComputePipelineLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/DescriptorBinding.hpp"
#include "Vulkan/DescriptorSetManager.hpp"
#include "Vulkan/DescriptorSets.hpp"
#include "Vulkan/ShaderModule.hpp"
#include <Utilities/FileUtils.h>

PixelManagementPipeline::PixelManagementPipeline(
	const Vulkan::SwapChain& swapChain,
	const VkBuffer dirtyObjectsBoundsBuffer,
	const VkBuffer dirtyObjectsCountBuffer,
	const VkBuffer pixelCleanStatusBuffer,
	const VkBuffer RayCountBuffer,
	const VkBuffer PixelWeightBuffer) : swapChain_(swapChain)
{
	const auto& device = swapChain.Device();

	const std::vector<Vulkan::DescriptorBinding> descriptorBindings =
	{
		// Dirty Object Bounds Buffer
		{0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},

		// Dirty Object Count Buffer
		{1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},

		// Clean Status Buffer
		{2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},

		// Ray Count Buffer
		{3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},

		// Pixel Weight Buffer
		{4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT},
	};

	descriptorSetManager_.reset(new Vulkan::DescriptorSetManager(device, descriptorBindings, swapChain.ImageViews().size()));
	auto& descriptorSets = descriptorSetManager_->DescriptorSets();

	for (uint32_t i = 0; i != swapChain.Images().size(); ++i)
	{
		// Dirty Object Bounds Buffer
		VkDescriptorBufferInfo DOBBInfo = {};
		DOBBInfo.buffer = dirtyObjectsBoundsBuffer;
		DOBBInfo.range = VK_WHOLE_SIZE;

		// Dirty Object Count Buffer
		VkDescriptorBufferInfo DOCBInfo = {};
		DOCBInfo.buffer = dirtyObjectsCountBuffer;
		DOCBInfo.range = VK_WHOLE_SIZE;

		// Clean Status Buffer
		VkDescriptorBufferInfo PCSBInfo = {};
		PCSBInfo.buffer = pixelCleanStatusBuffer;
		PCSBInfo.range = VK_WHOLE_SIZE;

		// Ray Count Buffer
		VkDescriptorBufferInfo RCBInfo = {};
		RCBInfo.buffer = RayCountBuffer;
		RCBInfo.range = VK_WHOLE_SIZE;

		// Pixel Weight Buffer
		VkDescriptorBufferInfo PWBInfo = {};
		PWBInfo.buffer = RayCountBuffer;
		PWBInfo.range = VK_WHOLE_SIZE;

		std::vector<VkWriteDescriptorSet> descriptorWrites =
		{
			descriptorSets.Bind(i, 0, DOBBInfo),
			descriptorSets.Bind(i, 1, DOCBInfo),
			descriptorSets.Bind(i, 2, PCSBInfo),
			descriptorSets.Bind(i, 3, RCBInfo),
			descriptorSets.Bind(i, 4, PWBInfo),
		};

		descriptorSets.UpdateDescriptors(i, descriptorWrites);
	}

	pipelineLayout_.reset(new class ComputePipelineLayout(device, descriptorSetManager_->DescriptorSetLayout()));

	const Vulkan::ShaderModule computeShader(device, FileUtils::getAssetsFolderPath().generic_string() + "/shaders/PixelManagement.comp.spv");

	VkComputePipelineCreateInfo  pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = pipelineLayout_->Handle();
	pipelineInfo.stage = computeShader.CreateShaderStage(VK_SHADER_STAGE_COMPUTE_BIT);

	Vulkan::Check(vkCreateComputePipelines(device.Handle(), nullptr, 1, &pipelineInfo, nullptr, &pipeline_), "create compute pipeline");
}

PixelManagementPipeline::~PixelManagementPipeline()
{
	if (pipeline_ != nullptr)
	{
		vkDestroyPipeline(swapChain_.Device().Handle(), pipeline_, nullptr);
		pipeline_ = nullptr;
	}

	pipelineLayout_.reset();
	descriptorSetManager_.reset();
}

VkDescriptorSet PixelManagementPipeline::DescriptorSet(const uint32_t index) const
{
	return descriptorSetManager_->DescriptorSets().Handle(index);
}
