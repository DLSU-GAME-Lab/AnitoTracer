#include "RayPicker.hpp"
#include "Vulkan/RayTracing/RayTracingProperties.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/RayTracing/DeviceProcedures.hpp"
#include "RayPickerUBO.hpp"

using namespace Vulkan::RayTracing;

static uint32_t alignTo(uint32_t v, uint32_t a) {
	return (v + a - 1) & ~(a - 1);
}

RayPicker::RayPicker(const DeviceProcedures& dp, const Vulkan::SwapChain& swapChain, Vulkan::CommandPool& cmdPool, const Vulkan::RayTracing::TopLevelAccelerationStructure& accelerationStructure,
	const std::vector<RayPickerUniformBuffer>& uniformBuffers, const Assets::Scene& scene, const Vulkan::RayTracing::RayTracingProperties& properties)
{
	resultBuffer.reset(new Vulkan::Buffer(swapChain.Device(), sizeof(Result), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	resultMemory.reset(new Vulkan::DeviceMemory(resultBuffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));

	stagingBuffer.reset(new Vulkan::Buffer(swapChain.Device(), sizeof(Result), VK_BUFFER_USAGE_TRANSFER_DST_BIT));
	stagingMemory.reset(new Vulkan::DeviceMemory(stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)));

	commandBuffers.reset(new Vulkan::CommandBuffers(cmdPool, static_cast<uint32_t>(swapChain.ImageViews().size())));

	rayPickerPipeline.reset(new RayPickerPipeline(dp, swapChain, accelerationStructure, *resultBuffer, uniformBuffers));

	const std::vector<ShaderBindingTable::Entry> rayGen = { {rayPickerPipeline->RayGenShaderIndex(), {}} };
	const std::vector<ShaderBindingTable::Entry> miss = { {rayPickerPipeline->MissShaderIndex(), {}} };
	const std::vector<ShaderBindingTable::Entry> hitGroup = { {rayPickerPipeline->HitGroupIndex(), {}} };

	rayPickerSBT.reset(new ShaderBindingTable(dp, *rayPickerPipeline, properties, rayGen, miss, hitGroup));
}

RayPicker::~RayPicker()
{
	rayPickerSBT.reset();
	rayPickerPipeline.reset();
	resultMemory.reset();
	stagingMemory.reset();
	resultBuffer.reset();
	stagingBuffer.reset();
}

RayPicker::Result RayPicker::pick(const Vulkan::RayTracing::DeviceProcedures& dp, const Vulkan::Device& device, glm::vec3 origin, glm::vec3 dir, const uint32_t imageIndex)
{
	PushConstantScreenPosition pushConstants;
	pushConstants.origin = glm::vec4(origin, 0);
	pushConstants.dir = glm::vec4(dir, 0);

	auto commandBuffer = commandBuffers->Begin(imageIndex);

	vkCmdFillBuffer(commandBuffer, resultBuffer->Handle(), 0, VK_WHOLE_SIZE, 0);

	VkBufferMemoryBarrier bufferBarrier = {};
	bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
	bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	bufferBarrier.buffer = resultBuffer->Handle();
	bufferBarrier.offset = 0;
	bufferBarrier.size = VK_WHOLE_SIZE;

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
		0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

	VkDescriptorSet descriptorSets[] = { rayPickerPipeline->DescriptorSet(imageIndex) };
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayPickerPipeline->Handle());
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayPickerPipeline->PipelineLayout().Handle(), 0, 1, descriptorSets, 0, nullptr);

	vkCmdPushConstants(commandBuffer, rayPickerPipeline->PipelineLayout().Handle(), VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		0, sizeof(PushConstantScreenPosition), &pushConstants);

	// Describe the shader binding table.
	VkStridedDeviceAddressRegionKHR raygenShaderBindingTable = {};
	raygenShaderBindingTable.deviceAddress = rayPickerSBT->RayGenDeviceAddress();
	raygenShaderBindingTable.stride = rayPickerSBT->RayGenEntrySize();
	raygenShaderBindingTable.size = rayPickerSBT->RayGenSize();

	VkStridedDeviceAddressRegionKHR missShaderBindingTable = {};
	missShaderBindingTable.deviceAddress = rayPickerSBT->MissDeviceAddress();
	missShaderBindingTable.stride = rayPickerSBT->MissEntrySize();
	missShaderBindingTable.size = rayPickerSBT->MissSize();

	VkStridedDeviceAddressRegionKHR hitShaderBindingTable = {};
	hitShaderBindingTable.deviceAddress = rayPickerSBT->HitGroupDeviceAddress();
	hitShaderBindingTable.stride = rayPickerSBT->HitGroupEntrySize();
	hitShaderBindingTable.size = rayPickerSBT->HitGroupSize();

	VkStridedDeviceAddressRegionKHR callableShaderBindingTable = {};

	// Execute ray tracing shaders.
	dp.vkCmdTraceRaysKHR(commandBuffer, &raygenShaderBindingTable, &missShaderBindingTable, &hitShaderBindingTable, &callableShaderBindingTable,
		1, 1, 1); // 1 x 1 x 1 pixel

	bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	bufferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = sizeof(Result);

	vkCmdCopyBuffer(commandBuffer, resultBuffer->Handle(), stagingBuffer->Handle(), 1, &copyRegion);

	bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	bufferBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
	bufferBarrier.buffer = stagingBuffer->Handle();

	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 0, nullptr, 1, &bufferBarrier, 0, nullptr);

	commandBuffers->End(imageIndex);

	VkFence fence;
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkCreateFence(device.Handle(), &fenceInfo, nullptr, &fence);

	// Submit command buffer
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(device.GraphicsQueue(), 1, &submitInfo, fence);
	vkWaitForFences(device.Handle(), 1, &fence, VK_TRUE, UINT64_MAX);

	Result result = {};
	void* data = stagingMemory->Map(0, sizeof(Result));
	memcpy(&result, data, sizeof(Result));
	stagingMemory->Unmap();

	vkDestroyFence(device.Handle(), fence, nullptr);

	return result;
}
