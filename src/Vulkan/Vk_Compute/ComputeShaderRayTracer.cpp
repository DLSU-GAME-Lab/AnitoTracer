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
	// CRITICAL: These bindings MUST match the shader exactly (assets/shaders/compute_raytrace.comp)
	const auto& device = swapChain.Device();
	const std::vector<DescriptorBinding> descriptorBindings =
	{
		// Binding 0: Camera uniform block
		{0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 1: Material buffer
		{1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 2: Light buffer
		{2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 3: Vertex buffer
		{3, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 4: Index buffer
		{4, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 5: Texture samplers (array)
		{5, static_cast<uint32_t>(scene.TextureSamplers().size()), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 6: Skybox sampler
		{6, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 7: Output image (storage)
		{7, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 8: Accumulation image (storage)
		{8, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 9: Capture image (storage)
		{9, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 10: Ray vertex buffer
		{10, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 11: Ray info buffer
		{11, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 12: Ray counter buffer
		{12, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},

		// Binding 13: Offsets buffer (per-object {indexOffset, vertexOffset})
		{13, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT},
	};

	descriptorSetManager_.reset(new DescriptorSetManager(device, descriptorBindings, uniformBuffers.size()));

	auto& descriptorSets = descriptorSetManager_->DescriptorSets();

	for (uint32_t i = 0; i != swapChain.Images().size(); ++i)
	{
		// Prepare all descriptor info structures

		// Binding 0: Camera uniform buffer
		VkDescriptorBufferInfo uniformBufferInfo = {};
		uniformBufferInfo.buffer = uniformBuffers[i].Buffer().Handle();
		uniformBufferInfo.range = VK_WHOLE_SIZE;

		// Binding 1: Material buffer
		VkDescriptorBufferInfo materialBufferInfo = {};
		materialBufferInfo.buffer = scene.MaterialBuffer().Handle();
		materialBufferInfo.range = VK_WHOLE_SIZE;

		// Binding 2: Lights buffer
		VkDescriptorBufferInfo lightsBufferInfo = {};
		lightsBufferInfo.buffer = scene.LightBuffer().Handle();
		lightsBufferInfo.range = VK_WHOLE_SIZE;

		// Binding 3: Vertex buffer
		VkDescriptorBufferInfo vertexBufferInfo = {};
		vertexBufferInfo.buffer = scene.VertexBuffer().Handle();
		vertexBufferInfo.range = VK_WHOLE_SIZE;

		// Binding 4: Index buffer
		VkDescriptorBufferInfo indexBufferInfo = {};
		indexBufferInfo.buffer = scene.IndexBuffer().Handle();
		indexBufferInfo.range = VK_WHOLE_SIZE;

		// Binding 5: Texture samplers (array)
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

		// Binding 6: Skybox sampler
		VkDescriptorImageInfo skyboxImageInfo = {};
		skyboxImageInfo.sampler = scene.SkyboxSampler();
		skyboxImageInfo.imageView = scene.SkyboxImageView();
		skyboxImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// Binding 7: Output image (storage)
		VkDescriptorImageInfo outputImageInfo = {};
		outputImageInfo.imageView = outputImageView.Handle();
		outputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		// Binding 8: Accumulation image (storage)
		VkDescriptorImageInfo accumulationImageInfo = {};
		accumulationImageInfo.imageView = accumulationImageView.Handle();
		accumulationImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		// Binding 9: Capture image (storage)
		VkDescriptorImageInfo captureImageInfo = {};
		captureImageInfo.imageView = outputImageViewS.Handle();
		captureImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		// Binding 10: Ray vertex buffer
		VkDescriptorBufferInfo rayVertexBufferInfo = {};
		rayVertexBufferInfo.buffer = rayScene.RayVertexBuffer().Handle();
		rayVertexBufferInfo.range = VK_WHOLE_SIZE;

		// Binding 11: Ray info buffer
		VkDescriptorBufferInfo rayInfoBufferInfo = {};
		rayInfoBufferInfo.buffer = rayScene.RayInfoBuffer().Handle();
		rayInfoBufferInfo.range = VK_WHOLE_SIZE;

		// Binding 12: Ray counter buffer
		VkDescriptorBufferInfo rayCounterBufferInfo = {};
		rayCounterBufferInfo.buffer = rayScene.RayCounterBuffer().Handle();
		rayCounterBufferInfo.range = VK_WHOLE_SIZE;

		// Binding 13: Offsets buffer
		VkDescriptorBufferInfo offsetsBufferInfo = {};
		offsetsBufferInfo.buffer = scene.OffsetsBuffer().Handle();
		offsetsBufferInfo.range = VK_WHOLE_SIZE;

		// Create write descriptor set array in the correct order
		std::vector<VkWriteDescriptorSet> descriptorWrites;

		// Binding 0: Camera UBO
		VkWriteDescriptorSet uniformBufferWrite = {};
		uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uniformBufferWrite.dstSet = descriptorSets.Handle(i);
		uniformBufferWrite.dstBinding = 0;
		uniformBufferWrite.dstArrayElement = 0;
		uniformBufferWrite.descriptorCount = 1;
		uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferWrite.pBufferInfo = &uniformBufferInfo;
		descriptorWrites.push_back(uniformBufferWrite);

		// Binding 1: Material buffer
		VkWriteDescriptorSet materialBufferWrite = {};
		materialBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		materialBufferWrite.dstSet = descriptorSets.Handle(i);
		materialBufferWrite.dstBinding = 1;
		materialBufferWrite.dstArrayElement = 0;
		materialBufferWrite.descriptorCount = 1;
		materialBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		materialBufferWrite.pBufferInfo = &materialBufferInfo;
		descriptorWrites.push_back(materialBufferWrite);

		// Binding 2: Lights buffer
		VkWriteDescriptorSet lightsBufferWrite = {};
		lightsBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		lightsBufferWrite.dstSet = descriptorSets.Handle(i);
		lightsBufferWrite.dstBinding = 2;
		lightsBufferWrite.dstArrayElement = 0;
		lightsBufferWrite.descriptorCount = 1;
		lightsBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		lightsBufferWrite.pBufferInfo = &lightsBufferInfo;
		descriptorWrites.push_back(lightsBufferWrite);

		// Binding 3: Vertex buffer
		VkWriteDescriptorSet vertexBufferWrite = {};
		vertexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vertexBufferWrite.dstSet = descriptorSets.Handle(i);
		vertexBufferWrite.dstBinding = 3;
		vertexBufferWrite.dstArrayElement = 0;
		vertexBufferWrite.descriptorCount = 1;
		vertexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vertexBufferWrite.pBufferInfo = &vertexBufferInfo;
		descriptorWrites.push_back(vertexBufferWrite);

		// Binding 4: Index buffer
		VkWriteDescriptorSet indexBufferWrite = {};
		indexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		indexBufferWrite.dstSet = descriptorSets.Handle(i);
		indexBufferWrite.dstBinding = 4;
		indexBufferWrite.dstArrayElement = 0;
		indexBufferWrite.descriptorCount = 1;
		indexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		indexBufferWrite.pBufferInfo = &indexBufferInfo;
		descriptorWrites.push_back(indexBufferWrite);

		// Binding 5: Texture samplers array
		VkWriteDescriptorSet textureWrite = {};
		textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		textureWrite.dstSet = descriptorSets.Handle(i);
		textureWrite.dstBinding = 5;
		textureWrite.dstArrayElement = 0;
		textureWrite.descriptorCount = static_cast<uint32_t>(textureImageInfos.size());
		textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureWrite.pImageInfo = textureImageInfos.data();
		descriptorWrites.push_back(textureWrite);

		// Binding 6: Skybox sampler
		VkWriteDescriptorSet skyboxWrite = {};
		skyboxWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		skyboxWrite.dstSet = descriptorSets.Handle(i);
		skyboxWrite.dstBinding = 6;
		skyboxWrite.dstArrayElement = 0;
		skyboxWrite.descriptorCount = 1;
		skyboxWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		skyboxWrite.pImageInfo = &skyboxImageInfo;
		descriptorWrites.push_back(skyboxWrite);

		// Binding 7: Output image storage
		VkWriteDescriptorSet outputImageWrite = {};
		outputImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		outputImageWrite.dstSet = descriptorSets.Handle(i);
		outputImageWrite.dstBinding = 7;
		outputImageWrite.dstArrayElement = 0;
		outputImageWrite.descriptorCount = 1;
		outputImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		outputImageWrite.pImageInfo = &outputImageInfo;
		descriptorWrites.push_back(outputImageWrite);

		// Binding 8: Accumulation image storage
		VkWriteDescriptorSet accumulationImageWrite = {};
		accumulationImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		accumulationImageWrite.dstSet = descriptorSets.Handle(i);
		accumulationImageWrite.dstBinding = 8;
		accumulationImageWrite.dstArrayElement = 0;
		accumulationImageWrite.descriptorCount = 1;
		accumulationImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		accumulationImageWrite.pImageInfo = &accumulationImageInfo;
		descriptorWrites.push_back(accumulationImageWrite);

		// Binding 9: Capture image storage
		VkWriteDescriptorSet captureImageWrite = {};
		captureImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		captureImageWrite.dstSet = descriptorSets.Handle(i);
		captureImageWrite.dstBinding = 9;
		captureImageWrite.dstArrayElement = 0;
		captureImageWrite.descriptorCount = 1;
		captureImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		captureImageWrite.pImageInfo = &captureImageInfo;
		descriptorWrites.push_back(captureImageWrite);

		// Binding 10: Ray vertex buffer
		VkWriteDescriptorSet rayVertexBufferWrite = {};
		rayVertexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		rayVertexBufferWrite.dstSet = descriptorSets.Handle(i);
		rayVertexBufferWrite.dstBinding = 10;
		rayVertexBufferWrite.dstArrayElement = 0;
		rayVertexBufferWrite.descriptorCount = 1;
		rayVertexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		rayVertexBufferWrite.pBufferInfo = &rayVertexBufferInfo;
		descriptorWrites.push_back(rayVertexBufferWrite);

		// Binding 11: Ray info buffer
		VkWriteDescriptorSet rayInfoBufferWrite = {};
		rayInfoBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		rayInfoBufferWrite.dstSet = descriptorSets.Handle(i);
		rayInfoBufferWrite.dstBinding = 11;
		rayInfoBufferWrite.dstArrayElement = 0;
		rayInfoBufferWrite.descriptorCount = 1;
		rayInfoBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		rayInfoBufferWrite.pBufferInfo = &rayInfoBufferInfo;
		descriptorWrites.push_back(rayInfoBufferWrite);

		// Binding 12: Ray counter buffer
		VkWriteDescriptorSet rayCounterBufferWrite = {};
		rayCounterBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		rayCounterBufferWrite.dstSet = descriptorSets.Handle(i);
		rayCounterBufferWrite.dstBinding = 12;
		rayCounterBufferWrite.dstArrayElement = 0;
		rayCounterBufferWrite.descriptorCount = 1;
		rayCounterBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		rayCounterBufferWrite.pBufferInfo = &rayCounterBufferInfo;
		descriptorWrites.push_back(rayCounterBufferWrite);

		// Binding 13: Offsets buffer
		VkWriteDescriptorSet offsetsBufferWrite = {};
		offsetsBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		offsetsBufferWrite.dstSet = descriptorSets.Handle(i);
		offsetsBufferWrite.dstBinding = 13;
		offsetsBufferWrite.dstArrayElement = 0;
		offsetsBufferWrite.descriptorCount = 1;
		offsetsBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		offsetsBufferWrite.pBufferInfo = &offsetsBufferInfo;
		descriptorWrites.push_back(offsetsBufferWrite);

		// Update all descriptors at once
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
