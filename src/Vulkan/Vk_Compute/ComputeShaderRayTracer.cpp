#include "ComputeShaderRayTracer.hpp"
#include "Assets/RayScene.hpp"
#include "Assets/Scene.hpp"
#include "Assets/UniformBuffer.hpp"
#include "Utilities/Exception.hpp"
#include "Utilities/FileUtils.h"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/DescriptorBinding.hpp"
#include "Vulkan/DescriptorSetManager.hpp"
#include "Vulkan/DescriptorSets.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/RayTracing/TopLevelAccelerationStructure.hpp"

namespace Vulkan::Compute {

ComputeShaderRayTracer::ComputeShaderRayTracer(
	const Vulkan::SwapChain& swapChain,
	const Vulkan::RayTracing::TopLevelAccelerationStructure& accelerationStructure,
	const Vulkan::ImageView& accumulationImageView,
	const Vulkan::ImageView& outputImageView,
	const Vulkan::ImageView& outputImageViewS,
	const std::vector<Assets::UniformBuffer>& uniformBuffers,
	const Assets::Scene& scene,
	const Assets::RayScene& rayScene) :
	swapChain_(swapChain)
{
	// Create descriptor pool/sets for compute shader
	const auto& device = swapChain.Device();
	const std::vector<DescriptorBinding> descriptorBindings =
	{
		// Top level acceleration structure.
		{0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_COMPUTE_BIT},

		// Image accumulation & output
		{1, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},
		{2, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},

		// Camera information & co
		{3, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Vertex buffer, Index buffer, Material buffer, Lights buffer, Offset buffer
		{4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
		{5, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
		{6, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
		{7, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
		{8, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Textures and image samplers
		{9, static_cast<uint32_t>(scene.TextureSamplers().size()), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},

		// The Procedural buffer.
		{10, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Skybox
		{11, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Ray Visualization Data
		{ 12, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
		{ 13, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },
		{ 14, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT },

		// Pre-Swizzled Image Buffer
		{15, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},
	};

	descriptorSetManager_.reset(new DescriptorSetManager(device, descriptorBindings, uniformBuffers.size()));

	auto& descriptorSets = descriptorSetManager_->DescriptorSets();

	for (uint32_t i = 0; i != swapChain.Images().size(); ++i)
	{
		// Top level acceleration structure.
		const auto accelerationStructureHandle = accelerationStructure.Handle();
		VkWriteDescriptorSetAccelerationStructureKHR structureInfo = {};
		structureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
		structureInfo.pNext = nullptr;
		structureInfo.accelerationStructureCount = 1;
		structureInfo.pAccelerationStructures = &accelerationStructureHandle;

		// Accumulation image
		VkDescriptorImageInfo accumulationImageInfo = {};
		accumulationImageInfo.imageView = accumulationImageView.Handle();
		accumulationImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		// Output image
		VkDescriptorImageInfo outputImageInfo = {};
		outputImageInfo.imageView = outputImageView.Handle();
		outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		// Uniform buffer
		VkDescriptorBufferInfo uniformBufferInfo = {};
		uniformBufferInfo.buffer = uniformBuffers[i].Buffer().Handle();
		uniformBufferInfo.range = VK_WHOLE_SIZE;

		// Write descriptors
		VkWriteDescriptorSet accelerationStructureWrite = {};
		accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		accelerationStructureWrite.pNext = &structureInfo;
		accelerationStructureWrite.dstSet = descriptorSets.Handle(i);
		accelerationStructureWrite.dstBinding = 0;
		accelerationStructureWrite.dstArrayElement = 0;
		accelerationStructureWrite.descriptorCount = 1;
		accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		accelerationStructureWrite.pImageInfo = nullptr;
		accelerationStructureWrite.pBufferInfo = nullptr;
		accelerationStructureWrite.pTexelBufferView = nullptr;

		VkWriteDescriptorSet accumulationImageWrite = {};
		accumulationImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		accumulationImageWrite.pNext = nullptr;
		accumulationImageWrite.dstSet = descriptorSets.Handle(i);
		accumulationImageWrite.dstBinding = 1;
		accumulationImageWrite.dstArrayElement = 0;
		accumulationImageWrite.descriptorCount = 1;
		accumulationImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		accumulationImageWrite.pImageInfo = &accumulationImageInfo;
		accumulationImageWrite.pBufferInfo = nullptr;
		accumulationImageWrite.pTexelBufferView = nullptr;

		VkWriteDescriptorSet outputImageWrite = {};
		outputImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		outputImageWrite.pNext = nullptr;
		outputImageWrite.dstSet = descriptorSets.Handle(i);
		outputImageWrite.dstBinding = 2;
		outputImageWrite.dstArrayElement = 0;
		outputImageWrite.descriptorCount = 1;
		outputImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		outputImageWrite.pImageInfo = &outputImageInfo;
		outputImageWrite.pBufferInfo = nullptr;
		outputImageWrite.pTexelBufferView = nullptr;

		VkWriteDescriptorSet uniformBufferWrite = {};
		uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformBufferWrite.pNext = nullptr;
		uniformBufferWrite.dstSet = descriptorSets.Handle(i);
		uniformBufferWrite.dstBinding = 3;
		uniformBufferWrite.dstArrayElement = 0;
		uniformBufferWrite.descriptorCount = 1;
		uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferWrite.pImageInfo = nullptr;
		uniformBufferWrite.pBufferInfo = &uniformBufferInfo;
		uniformBufferWrite.pTexelBufferView = nullptr;

		// Vertex buffer
		VkDescriptorBufferInfo vertexBufferInfo = {};
		vertexBufferInfo.buffer = scene.VertexBuffer().Handle();
		vertexBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet vertexBufferWrite = {};
		vertexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vertexBufferWrite.pNext = nullptr;
		vertexBufferWrite.dstSet = descriptorSets.Handle(i);
		vertexBufferWrite.dstBinding = 4;
		vertexBufferWrite.dstArrayElement = 0;
		vertexBufferWrite.descriptorCount = 1;
		vertexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vertexBufferWrite.pImageInfo = nullptr;
		vertexBufferWrite.pBufferInfo = &vertexBufferInfo;
		vertexBufferWrite.pTexelBufferView = nullptr;

		// Index buffer
		VkDescriptorBufferInfo indexBufferInfo = {};
		indexBufferInfo.buffer = scene.IndexBuffer().Handle();
		indexBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet indexBufferWrite = {};
		indexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		indexBufferWrite.pNext = nullptr;
		indexBufferWrite.dstSet = descriptorSets.Handle(i);
		indexBufferWrite.dstBinding = 5;
		indexBufferWrite.dstArrayElement = 0;
		indexBufferWrite.descriptorCount = 1;
		indexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		indexBufferWrite.pImageInfo = nullptr;
		indexBufferWrite.pBufferInfo = &indexBufferInfo;
		indexBufferWrite.pTexelBufferView = nullptr;

		// Material buffer
		VkDescriptorBufferInfo materialBufferInfo = {};
		materialBufferInfo.buffer = scene.MaterialBuffer().Handle();
		materialBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet materialBufferWrite = {};
		materialBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		materialBufferWrite.pNext = nullptr;
		materialBufferWrite.dstSet = descriptorSets.Handle(i);
		materialBufferWrite.dstBinding = 6;
		materialBufferWrite.dstArrayElement = 0;
		materialBufferWrite.descriptorCount = 1;
		materialBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		materialBufferWrite.pImageInfo = nullptr;
		materialBufferWrite.pBufferInfo = &materialBufferInfo;
		materialBufferWrite.pTexelBufferView = nullptr;

		// Lights buffer
		VkDescriptorBufferInfo lightsBufferInfo = {};
		lightsBufferInfo.buffer = scene.LightBuffer().Handle();
		lightsBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet lightsBufferWrite = {};
		lightsBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		lightsBufferWrite.pNext = nullptr;
		lightsBufferWrite.dstSet = descriptorSets.Handle(i);
		lightsBufferWrite.dstBinding = 7;
		lightsBufferWrite.dstArrayElement = 0;
		lightsBufferWrite.descriptorCount = 1;
		lightsBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		lightsBufferWrite.pImageInfo = nullptr;
		lightsBufferWrite.pBufferInfo = &lightsBufferInfo;
		lightsBufferWrite.pTexelBufferView = nullptr;

		// Offset buffer
		VkDescriptorBufferInfo offsetBufferInfo = {};
		offsetBufferInfo.buffer = scene.OffsetsBuffer().Handle();
		offsetBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet offsetBufferWrite = {};
		offsetBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		offsetBufferWrite.pNext = nullptr;
		offsetBufferWrite.dstSet = descriptorSets.Handle(i);
		offsetBufferWrite.dstBinding = 8;
		offsetBufferWrite.dstArrayElement = 0;
		offsetBufferWrite.descriptorCount = 1;
		offsetBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		offsetBufferWrite.pImageInfo = nullptr;
		offsetBufferWrite.pBufferInfo = &offsetBufferInfo;
		offsetBufferWrite.pTexelBufferView = nullptr;

		// Texture samplers
		const auto& textureImageViews = scene.TextureImageViews();
		const auto& textureSamplers = scene.TextureSamplers();
		std::vector<VkDescriptorImageInfo> textureImageInfos;
		for (size_t j = 0; j < textureSamplers.size(); ++j)
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.sampler = textureSamplers[j];
			imageInfo.imageView = textureImageViews[j];
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			textureImageInfos.push_back(imageInfo);
		}

		VkWriteDescriptorSet textureWrite = {};
		textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		textureWrite.pNext = nullptr;
		textureWrite.dstSet = descriptorSets.Handle(i);
		textureWrite.dstBinding = 9;
		textureWrite.dstArrayElement = 0;
		textureWrite.descriptorCount = static_cast<uint32_t>(textureImageInfos.size());
		textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureWrite.pImageInfo = textureImageInfos.data();
		textureWrite.pBufferInfo = nullptr;
		textureWrite.pTexelBufferView = nullptr;

		// Procedural buffer
		VkDescriptorBufferInfo proceduralBufferInfo = {};
		proceduralBufferInfo.buffer = scene.ProceduralBuffer().Handle();
		proceduralBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet proceduralBufferWrite = {};
		proceduralBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		proceduralBufferWrite.pNext = nullptr;
		proceduralBufferWrite.dstSet = descriptorSets.Handle(i);
		proceduralBufferWrite.dstBinding = 10;
		proceduralBufferWrite.dstArrayElement = 0;
		proceduralBufferWrite.descriptorCount = 1;
		proceduralBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		proceduralBufferWrite.pImageInfo = nullptr;
		proceduralBufferWrite.pBufferInfo = &proceduralBufferInfo;
		proceduralBufferWrite.pTexelBufferView = nullptr;

		// Skybox
		VkDescriptorImageInfo skyboxImageInfo = {};
		skyboxImageInfo.sampler = scene.SkyboxSampler();
		skyboxImageInfo.imageView = scene.SkyboxImageView();
		skyboxImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet skyboxWrite = {};
		skyboxWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		skyboxWrite.pNext = nullptr;
		skyboxWrite.dstSet = descriptorSets.Handle(i);
		skyboxWrite.dstBinding = 11;
		skyboxWrite.dstArrayElement = 0;
		skyboxWrite.descriptorCount = 1;
		skyboxWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		skyboxWrite.pImageInfo = &skyboxImageInfo;
		skyboxWrite.pBufferInfo = nullptr;
		skyboxWrite.pTexelBufferView = nullptr;

		// Ray counter buffer
		VkDescriptorBufferInfo rayCounterBufferInfo = {};
		rayCounterBufferInfo.buffer = rayScene.RayCounterBuffer().Handle();
		rayCounterBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet rayCounterBufferWrite = {};
		rayCounterBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		rayCounterBufferWrite.pNext = nullptr;
		rayCounterBufferWrite.dstSet = descriptorSets.Handle(i);
		rayCounterBufferWrite.dstBinding = 12;
		rayCounterBufferWrite.dstArrayElement = 0;
		rayCounterBufferWrite.descriptorCount = 1;
		rayCounterBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		rayCounterBufferWrite.pImageInfo = nullptr;
		rayCounterBufferWrite.pBufferInfo = &rayCounterBufferInfo;
		rayCounterBufferWrite.pTexelBufferView = nullptr;

		// Ray vertex buffer
		VkDescriptorBufferInfo rayVertexBufferInfo = {};
		rayVertexBufferInfo.buffer = rayScene.RayVertexBuffer().Handle();
		rayVertexBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet rayVertexBufferWrite = {};
		rayVertexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		rayVertexBufferWrite.pNext = nullptr;
		rayVertexBufferWrite.dstSet = descriptorSets.Handle(i);
		rayVertexBufferWrite.dstBinding = 13;
		rayVertexBufferWrite.dstArrayElement = 0;
		rayVertexBufferWrite.descriptorCount = 1;
		rayVertexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		rayVertexBufferWrite.pImageInfo = nullptr;
		rayVertexBufferWrite.pBufferInfo = &rayVertexBufferInfo;
		rayVertexBufferWrite.pTexelBufferView = nullptr;

		// Ray info buffer
		VkDescriptorBufferInfo rayInfoBufferInfo = {};
		rayInfoBufferInfo.buffer = rayScene.RayInfoBuffer().Handle();
		rayInfoBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet rayInfoBufferWrite = {};
		rayInfoBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		rayInfoBufferWrite.pNext = nullptr;
		rayInfoBufferWrite.dstSet = descriptorSets.Handle(i);
		rayInfoBufferWrite.dstBinding = 14;
		rayInfoBufferWrite.dstArrayElement = 0;
		rayInfoBufferWrite.descriptorCount = 1;
		rayInfoBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		rayInfoBufferWrite.pImageInfo = nullptr;
		rayInfoBufferWrite.pBufferInfo = &rayInfoBufferInfo;
		rayInfoBufferWrite.pTexelBufferView = nullptr;

		// Output image (swizzled/capture)
		VkDescriptorImageInfo outputImageSInfo = {};
		outputImageSInfo.imageView = outputImageViewS.Handle();
		outputImageSInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet outputImageSWrite = {};
		outputImageSWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		outputImageSWrite.pNext = nullptr;
		outputImageSWrite.dstSet = descriptorSets.Handle(i);
		outputImageSWrite.dstBinding = 15;
		outputImageSWrite.dstArrayElement = 0;
		outputImageSWrite.descriptorCount = 1;
		outputImageSWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		outputImageSWrite.pImageInfo = &outputImageSInfo;
		outputImageSWrite.pBufferInfo = nullptr;
		outputImageSWrite.pTexelBufferView = nullptr;

		std::vector<VkWriteDescriptorSet> descriptorWrites;
		descriptorWrites.push_back(accelerationStructureWrite);
		descriptorWrites.push_back(accumulationImageWrite);
		descriptorWrites.push_back(outputImageWrite);
		descriptorWrites.push_back(uniformBufferWrite);
		descriptorWrites.push_back(vertexBufferWrite);
		descriptorWrites.push_back(indexBufferWrite);
		descriptorWrites.push_back(materialBufferWrite);
		descriptorWrites.push_back(lightsBufferWrite);
		descriptorWrites.push_back(offsetBufferWrite);
		descriptorWrites.push_back(textureWrite);
		descriptorWrites.push_back(proceduralBufferWrite);
		descriptorWrites.push_back(skyboxWrite);
		descriptorWrites.push_back(rayCounterBufferWrite);
		descriptorWrites.push_back(rayVertexBufferWrite);
		descriptorWrites.push_back(rayInfoBufferWrite);
		descriptorWrites.push_back(outputImageSWrite);

		vkUpdateDescriptorSets(device.Handle(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	// Create pipeline layout
	pipelineLayout_.reset(new Vulkan::PipelineLayout(device, descriptorSetManager_->DescriptorSetLayout()));

	// Create compute pipeline
	auto shaderPath = FileUtils::getAssetsFolderPath().generic_string() + "/shaders/compute_raytrace.comp.spv";
	std::cout << "Loading compute shader from: " << shaderPath << std::endl;
	const Vulkan::ShaderModule computeShader(device, shaderPath);

	VkPipelineShaderStageCreateInfo computeShaderStage = computeShader.CreateShaderStage(VK_SHADER_STAGE_COMPUTE_BIT);

	VkComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.flags = 0;
	pipelineInfo.stage = computeShaderStage;
	pipelineInfo.layout = pipelineLayout_->Handle();
	pipelineInfo.basePipelineHandle = nullptr;
	pipelineInfo.basePipelineIndex = 0;

	Check(vkCreateComputePipelines(device.Handle(), nullptr, 1, &pipelineInfo, nullptr, &pipeline_),
		"create compute pipeline");
}

ComputeShaderRayTracer::~ComputeShaderRayTracer()
{
	if (pipeline_ != nullptr)
	{
		vkDestroyPipeline(swapChain_.Device().Handle(), pipeline_, nullptr);
		pipeline_ = nullptr;
	}

	pipelineLayout_.reset();
	descriptorSetManager_.reset();
}

VkDescriptorSet ComputeShaderRayTracer::DescriptorSet(const uint32_t index) const
{
	return descriptorSetManager_->DescriptorSets().Handle(index);
}

void ComputeShaderRayTracer::Dispatch(VkCommandBuffer commandBuffer, uint32_t imageIndex, const VkExtent2D& extent)
{
	// Bind compute pipeline and descriptor set
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
	const VkDescriptorSet descriptorSet = DescriptorSet(imageIndex);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout_->Handle(), 0, 1, &descriptorSet, 0, nullptr);

	// Dispatch compute shader
	// Assuming 8x8 local work group size in the compute shader
	const uint32_t groupSizeX = (extent.width + 7) / 8;
	const uint32_t groupSizeY = (extent.height + 7) / 8;

	vkCmdDispatch(commandBuffer, groupSizeX, groupSizeY, 1);
}

}
