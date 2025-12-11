#include "RayPickerPipeline.hpp"
#include "Vulkan/RayTracing/DeviceProcedures.hpp"
#include "Vulkan/RayTracing/TopLevelAccelerationStructure.hpp"
#include "Vulkan/DescriptorSetManager.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Buffer.hpp"
#include "RayPickerPipelineLayout.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/DescriptorSets.hpp"
#include "Assets/Scene.hpp"
#include "RayPickerUBO.hpp"
#include <Utilities/FileUtils.h>

namespace Vulkan::RayTracing
{
	namespace
	{
		ShaderModule LoadShader(const Device& device, const std::string& filename)
		{
			return ShaderModule(device, filename);
		}
	}

	RayPickerPipeline::RayPickerPipeline(
		const DeviceProcedures& deviceProcedures,
		const SwapChain& swapChain,
		const TopLevelAccelerationStructure& accelerationStructure,
		const Buffer& resultBuffer,
		const std::vector<RayPickerUniformBuffer>& uniformBuffers) :
		swapChain_(swapChain)
	{
		const auto& device = swapChain.Device();
		const std::vector<DescriptorBinding> descriptorBindings =
		{
			// Top level acceleration structure.
			{0, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_RAYGEN_BIT_KHR },
			// Result output buffer
			{1, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR },
			// Uniform Buffer Object
			{2, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_RAYGEN_BIT_KHR}
		};

		descriptorSetManager_.reset(new DescriptorSetManager(device, descriptorBindings, uniformBuffers.size()));
		auto& descriptorSets = descriptorSetManager_->DescriptorSets();

		for (uint32_t i = 0; i != swapChain.ImageViews().size(); ++i)
		{
			// TLAS
			const auto accelerationStructureHandle = accelerationStructure.Handle();
			VkWriteDescriptorSetAccelerationStructureKHR structureInfo = {};
			structureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
			structureInfo.accelerationStructureCount = 1;
			structureInfo.pAccelerationStructures = &accelerationStructureHandle;

			// Picker output image
			VkDescriptorBufferInfo resultOutputBuffer = {};
			resultOutputBuffer.buffer = resultBuffer.Handle();
			resultOutputBuffer.offset = 0;
			resultOutputBuffer.range = VK_WHOLE_SIZE;

			// Uniform buffer
			VkDescriptorBufferInfo uniformBufferInfo = {};
			uniformBufferInfo.buffer = uniformBuffers[i].Buffer().Handle();
			uniformBufferInfo.range = VK_WHOLE_SIZE;

			std::vector<VkWriteDescriptorSet> descriptorWrites =
			{
				descriptorSets.Bind(i, 0, structureInfo),
				descriptorSets.Bind(i, 1, resultOutputBuffer),
				descriptorSets.Bind(i, 2, uniformBufferInfo),
			};

			descriptorSets.UpdateDescriptors(i, descriptorWrites);
		}

		pipelineLayout_.reset(new class RayPickerPipelineLayout(device, descriptorSetManager_->DescriptorSetLayout()));

		// Load picker-specific shaders
		const ShaderModule pickerRayGenShader(LoadShader(device, FileUtils::getAssetsFolderPath().generic_string() + "/shaders/RayPicker.rgen.spv"));
		const ShaderModule pickerMissShader(LoadShader(device, FileUtils::getAssetsFolderPath().generic_string() + "/shaders/RayPicker.rmiss.spv"));
		const ShaderModule pickerClosestHitShader(LoadShader(device, FileUtils::getAssetsFolderPath().generic_string() + "/shaders/RayPicker.rchit.spv"));

		// Shader stages
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages =
		{
			pickerRayGenShader.CreateShaderStage(VK_SHADER_STAGE_RAYGEN_BIT_KHR),
			pickerMissShader.CreateShaderStage(VK_SHADER_STAGE_MISS_BIT_KHR),
			pickerClosestHitShader.CreateShaderStage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
		};

		// Shader groups
		VkRayTracingShaderGroupCreateInfoKHR rayGenGroup = {};
		rayGenGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		rayGenGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		rayGenGroup.generalShader = 0;  // pickerRayGenShader
		rayGenGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		rayGenGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		rayGenGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		rayGenIndex_ = 0;

		VkRayTracingShaderGroupCreateInfoKHR missGroup = {};
		missGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		missGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		missGroup.generalShader = 1;  // pickerMissShader
		missGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		missGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		missGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		missIndex_ = 1;

		VkRayTracingShaderGroupCreateInfoKHR hitGroup = {};
		hitGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		hitGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		hitGroup.generalShader = VK_SHADER_UNUSED_KHR;
		hitGroup.closestHitShader = 2;  // pickerClosestHitShader
		hitGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		hitGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		hitGroupIndex_ = 2;

		std::vector<VkRayTracingShaderGroupCreateInfoKHR> groups =
		{
			rayGenGroup,
			missGroup,
			hitGroup
		};

		// Create ray tracing pipeline
		VkRayTracingPipelineCreateInfoKHR pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
		pipelineInfo.pGroups = groups.data();
		pipelineInfo.maxPipelineRayRecursionDepth = 1;  // No recursion needed for picking
		pipelineInfo.layout = pipelineLayout_->Handle();

		Check(deviceProcedures.vkCreateRayTracingPipelinesKHR(device.Handle(), nullptr, nullptr, 1, &pipelineInfo, nullptr, &pipeline_),
			"create picker ray tracing pipeline");
	}

	RayPickerPipeline::~RayPickerPipeline()
	{
		pipelineLayout_.reset();
		descriptorSetManager_.reset();
	}

	VkDescriptorSet RayPickerPipeline::DescriptorSet(uint32_t index) const
	{
		return descriptorSetManager_->DescriptorSets().Handle(index);
	}
}