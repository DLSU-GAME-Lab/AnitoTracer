#include "Application.hpp"
#include "BottomLevelAccelerationStructure.hpp"
#include "DeviceProcedures.hpp"
#include "RayTracingPipeline.hpp"
#include "ShaderBindingTable.hpp"
#include "TopLevelAccelerationStructure.hpp"
#include "Assets/Model.hpp"
#include "Assets/Scene.hpp"
#include "Utilities/Glm.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/BufferUtil.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageMemoryBarrier.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/SingleTimeCommands.hpp"
#include "Vulkan/SwapChain.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <numeric>

#include "UI/UIManager.h"


namespace Vulkan::RayTracing {

namespace
{
	template <class TAccelerationStructure>
	VkAccelerationStructureBuildSizesInfoKHR GetTotalRequirements(const std::vector<TAccelerationStructure>& accelerationStructures)
	{
		VkAccelerationStructureBuildSizesInfoKHR total{};

		for (const auto& accelerationStructure : accelerationStructures)
		{
			total.accelerationStructureSize += accelerationStructure.BuildSizes().accelerationStructureSize;
			total.buildScratchSize += accelerationStructure.BuildSizes().buildScratchSize;
			total.updateScratchSize += accelerationStructure.BuildSizes().updateScratchSize;
		}

		return total;
	}
}

Application::Application(const WindowConfig& windowConfig, const VkPresentModeKHR presentMode, const bool enableValidationLayers) :
	Vulkan::Application(windowConfig, presentMode, enableValidationLayers)
{
}

Application::~Application()
{
	Application::DeleteSwapChain();
	DeleteAccelerationStructures();

	rayTracingProperties_.reset();
	deviceProcedures_.reset();
}

void Application::SetPhysicalDevice(
	VkPhysicalDevice physicalDevice,
	std::vector<const char*>& requiredExtensions,
	VkPhysicalDeviceFeatures& deviceFeatures,
	void* nextDeviceFeatures)
{
	// Required extensions.
	requiredExtensions.insert(requiredExtensions.end(),
	{	
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
	});

	// Required device features.
	VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {};
	bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
	bufferDeviceAddressFeatures.pNext = nextDeviceFeatures;
	bufferDeviceAddressFeatures.bufferDeviceAddress = true;

	VkPhysicalDeviceDescriptorIndexingFeatures indexingFeatures = {};
	indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	indexingFeatures.pNext = &bufferDeviceAddressFeatures;
	indexingFeatures.runtimeDescriptorArray = true;
	indexingFeatures.shaderSampledImageArrayNonUniformIndexing = true;

	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
	accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
	accelerationStructureFeatures.pNext = &indexingFeatures;
	accelerationStructureFeatures.accelerationStructure = true;
	
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures = {};
	rayTracingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
	rayTracingFeatures.pNext = &accelerationStructureFeatures;
	rayTracingFeatures.rayTracingPipeline = true;

	Vulkan::Application::SetPhysicalDevice(physicalDevice, requiredExtensions, deviceFeatures, &rayTracingFeatures);
}

void Application::OnDeviceSet()
{
	Vulkan::Application::OnDeviceSet();

	deviceProcedures_.reset(new DeviceProcedures(Device()));
	rayTracingProperties_.reset(new RayTracingProperties(Device()));
}

void Application::CreateAccelerationStructures()
{
	const auto timer = std::chrono::high_resolution_clock::now();

	SingleTimeCommands::Submit(CommandPool(), [this](VkCommandBuffer commandBuffer)
	{
		CreateBottomLevelStructures(commandBuffer);
		CreateTopLevelStructures(commandBuffer);
	});

	topScratchBuffer_.reset();
	topScratchBufferMemory_.reset();
	bottomScratchBuffer_.reset();
	bottomScratchBufferMemory_.reset();

	const auto elapsed = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - timer).count();
	std::cout << "- built acceleration structures in " << elapsed << "s" << std::endl;
}

void Application::DeleteAccelerationStructures()
{
	topAs_.clear();
	instancesBuffer_.reset();
	instancesBufferMemory_.reset();
	topScratchBuffer_.reset();
	topScratchBufferMemory_.reset();
	topBuffer_.reset();
	topBufferMemory_.reset();

	bottomAs_.clear();
	bottomScratchBuffer_.reset();
	bottomScratchBufferMemory_.reset();
	bottomBuffer_.reset();
	bottomBufferMemory_.reset();
}

void Application::CreateSwapChain()
{
	Vulkan::Application::CreateSwapChain();

	CreateOutputImage();

	rayTracingPipeline_.reset(new RayTracingPipeline(*deviceProcedures_, SwapChain(), topAs_[0], *accumulationImageView_, *outputImageView_, UniformBuffers(), GetScene(), GetRayScene()));

	const std::vector<ShaderBindingTable::Entry> rayGenPrograms = { {rayTracingPipeline_->RayGenShaderIndex(), {}} };
	const std::vector<ShaderBindingTable::Entry> missPrograms = { {rayTracingPipeline_->MissShaderIndex(), {}} };
	const std::vector<ShaderBindingTable::Entry> hitGroups = { {rayTracingPipeline_->TriangleHitGroupIndex(), {}}, {rayTracingPipeline_->ProceduralHitGroupIndex(), {}} };

	shaderBindingTable_.reset(new ShaderBindingTable(*deviceProcedures_, *rayTracingPipeline_, *rayTracingProperties_, rayGenPrograms, missPrograms, hitGroups));
}

void Application::DeleteSwapChain()
{
	shaderBindingTable_.reset();
	rayTracingPipeline_.reset();
	outputImageView_.reset();
	outputImage_.reset();
	outputImageMemory_.reset();
	accumulationImageView_.reset();
	accumulationImage_.reset();
	accumulationImageMemory_.reset();

	Vulkan::Application::DeleteSwapChain();
}

void Application::Render(VkCommandBuffer commandBuffer, const uint32_t imageIndex)
{
	const auto extent = SwapChain().Extent();

	VkDescriptorSet descriptorSets[] = { rayTracingPipeline_->DescriptorSet(imageIndex) };

	VkImageSubresourceRange subresourceRange = {};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.baseArrayLayer = 0;
	subresourceRange.layerCount = 1;

	// Acquire destination images for rendering.
	ImageMemoryBarrier::Insert(commandBuffer, accumulationImage_->Handle(), subresourceRange, 0,
		VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	ImageMemoryBarrier::Insert(commandBuffer, outputImage_->Handle(), subresourceRange, 0,
		VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	// Bind ray tracing pipeline.
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipeline_->Handle());
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipeline_->PipelineLayout().Handle(), 0, 1, descriptorSets, 0, nullptr);

	// Describe the shader binding table.
	VkStridedDeviceAddressRegionKHR raygenShaderBindingTable = {};
	raygenShaderBindingTable.deviceAddress = shaderBindingTable_->RayGenDeviceAddress();
	raygenShaderBindingTable.stride = shaderBindingTable_->RayGenEntrySize();
	raygenShaderBindingTable.size = shaderBindingTable_->RayGenSize();

	VkStridedDeviceAddressRegionKHR missShaderBindingTable = {};
	missShaderBindingTable.deviceAddress = shaderBindingTable_->MissDeviceAddress();
	missShaderBindingTable.stride = shaderBindingTable_->MissEntrySize();
	missShaderBindingTable.size = shaderBindingTable_->MissSize();

	VkStridedDeviceAddressRegionKHR hitShaderBindingTable = {};
	hitShaderBindingTable.deviceAddress = shaderBindingTable_->HitGroupDeviceAddress();
	hitShaderBindingTable.stride = shaderBindingTable_->HitGroupEntrySize();
	hitShaderBindingTable.size = shaderBindingTable_->HitGroupSize();

	VkStridedDeviceAddressRegionKHR callableShaderBindingTable = {};

	// Execute ray tracing shaders.
	deviceProcedures_->vkCmdTraceRaysKHR(commandBuffer,
		&raygenShaderBindingTable, &missShaderBindingTable, &hitShaderBindingTable, &callableShaderBindingTable,
		extent.width, extent.height, 1);

	// Acquire output image and swap-chain image for copying.
	ImageMemoryBarrier::Insert(commandBuffer, outputImage_->Handle(), subresourceRange, 
		VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

	ImageMemoryBarrier::Insert(commandBuffer, SwapChain().Images()[imageIndex], subresourceRange, 0,
		VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// Copy output image into swap-chain image.
	VkImageCopy copyRegion;
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { extent.width, extent.height, 1 };

	vkCmdCopyImage(commandBuffer,
		outputImage_->Handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		SwapChain().Images()[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1, &copyRegion);


	ImageMemoryBarrier::Insert(commandBuffer, SwapChain().Images()[imageIndex], subresourceRange, VK_ACCESS_TRANSFER_WRITE_BIT,
		0, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// Viewport test code
	//UIManager::getInstance()->m_Dset = descriptorSets[0];
	//UIManager::getInstance()->images = &SwapChain().Images();
	//UIManager::getInstance()->imageView = outputImageView_.get();
	//UIManager::getInstance()->image = outputImage_.get();
}

void Application::CreateBottomLevelStructures(VkCommandBuffer commandBuffer)
{
	const auto& scene = GetScene();
	const auto& debugUtils = Device().DebugUtils();
	
	// Bottom level acceleration structure
	// Triangles via vertex buffers. Procedurals via AABBs.
	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;
	uint32_t aabbOffset = 0;

	for (const auto& model : scene.Models())
	{
		const auto vertexCount = static_cast<uint32_t>(model.NumberOfVertices());
		const auto indexCount = static_cast<uint32_t>(model.NumberOfIndices());
		BottomLevelGeometry geometries;
		
		model.Procedural()
			? geometries.AddGeometryAabb(scene, aabbOffset, 1, true)
			: geometries.AddGeometryTriangles(scene, vertexOffset, vertexCount, indexOffset, indexCount, true);

		bottomAs_.emplace_back(*deviceProcedures_, *rayTracingProperties_, geometries);

		vertexOffset += vertexCount * sizeof(Assets::Vertex);
		indexOffset += indexCount * sizeof(uint32_t);
		aabbOffset += sizeof(VkAabbPositionsKHR);
	}

	// Allocate the structures memory.
	const auto total = GetTotalRequirements(bottomAs_);

	bottomBuffer_.reset(new Buffer(Device(), total.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
	bottomBufferMemory_.reset(new DeviceMemory(bottomBuffer_->AllocateMemory(VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
	bottomScratchBuffer_.reset(new Buffer(Device(), total.buildScratchSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
	bottomScratchBufferMemory_.reset(new DeviceMemory(bottomScratchBuffer_->AllocateMemory(VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));

	debugUtils.SetObjectName(bottomBuffer_->Handle(), "BLAS Buffer");
	debugUtils.SetObjectName(bottomBufferMemory_->Handle(), "BLAS Memory");
	debugUtils.SetObjectName(bottomScratchBuffer_->Handle(), "BLAS Scratch Buffer");
	debugUtils.SetObjectName(bottomScratchBufferMemory_->Handle(), "BLAS Scratch Memory");

	// Generate the structures.
	VkDeviceSize resultOffset = 0;
	VkDeviceSize scratchOffset = 0;

	for (size_t i = 0; i != bottomAs_.size(); ++i)
	{
		bottomAs_[i].Generate(commandBuffer, *bottomScratchBuffer_, scratchOffset, *bottomBuffer_, resultOffset);
		
		resultOffset += bottomAs_[i].BuildSizes().accelerationStructureSize;
		scratchOffset += bottomAs_[i].BuildSizes().buildScratchSize;

		debugUtils.SetObjectName(bottomAs_[i].Handle(), ("BLAS #" + std::to_string(i)).c_str());
	}
}

void Application::CreateTopLevelStructures(VkCommandBuffer commandBuffer)
{
	const auto& scene = GetScene();
	const auto& debugUtils = Device().DebugUtils();

	// Top level acceleration structure
	std::vector<VkAccelerationStructureInstanceKHR> instances;

	// Hit group 0: triangles
	// Hit group 1: procedurals
	uint32_t instanceId = 0;

	for (const auto& model : scene.Models())
	{
		instances.push_back(TopLevelAccelerationStructure::CreateInstance(
			bottomAs_[instanceId], glm::mat4(1), instanceId, model.Procedural() ? 1 : 0));
		instanceId++;
	}

	// Create and copy instances buffer (do it in a separate one-time synchronous command buffer).
	BufferUtil::CreateDeviceBuffer(CommandPool(), "TLAS Instances", VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, instances, instancesBuffer_, instancesBufferMemory_);

	// Memory barrier for the bottom level acceleration structure builds.
	AccelerationStructure::MemoryBarrier(commandBuffer);
	
	topAs_.emplace_back(*deviceProcedures_, *rayTracingProperties_, instancesBuffer_->GetDeviceAddress(), static_cast<uint32_t>(instances.size()));

	// Allocate the structure memory.
	const auto total = GetTotalRequirements(topAs_);

	topBuffer_.reset(new Buffer(Device(), total.accelerationStructureSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR));
	topBufferMemory_.reset(new DeviceMemory(topBuffer_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));

	topScratchBuffer_.reset(new Buffer(Device(), total.buildScratchSize, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
	topScratchBufferMemory_.reset(new DeviceMemory(topScratchBuffer_->AllocateMemory(VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));

	
	debugUtils.SetObjectName(topBuffer_->Handle(), "TLAS Buffer");
	debugUtils.SetObjectName(topBufferMemory_->Handle(), "TLAS Memory");
	debugUtils.SetObjectName(topScratchBuffer_->Handle(), "TLAS Scratch Buffer");
	debugUtils.SetObjectName(topScratchBufferMemory_->Handle(), "TLAS Scratch Memory");
	debugUtils.SetObjectName(instancesBuffer_->Handle(), "TLAS Instances Buffer");
	debugUtils.SetObjectName(instancesBufferMemory_->Handle(), "TLAS Instances Memory");

	// Generate the structures.
	topAs_[0].Generate(commandBuffer, *topScratchBuffer_, 0, *topBuffer_, 0);

	debugUtils.SetObjectName(topAs_[0].Handle(), "TLAS");
}

void Application::CreateOutputImage()
{
	const auto extent = SwapChain().Extent();
	const auto format = SwapChain().Format();
	const auto tiling = VK_IMAGE_TILING_OPTIMAL;

	accumulationImage_.reset(new Image(Device(), extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_STORAGE_BIT));
	accumulationImageMemory_.reset(new DeviceMemory(accumulationImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
	accumulationImageView_.reset(new ImageView(Device(), accumulationImage_->Handle(), VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT));

	outputImage_.reset(new Image(Device(), extent, format, tiling, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT));
	outputImageMemory_.reset(new DeviceMemory(outputImage_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
	outputImageView_.reset(new ImageView(Device(), outputImage_->Handle(), format, VK_IMAGE_ASPECT_COLOR_BIT));

	const auto& debugUtils = Device().DebugUtils();
	
	debugUtils.SetObjectName(accumulationImage_->Handle(), "Accumulation Image");
	debugUtils.SetObjectName(accumulationImageMemory_->Handle(), "Accumulation Image Memory");
	debugUtils.SetObjectName(accumulationImageView_->Handle(), "Accumulation ImageView");
	
	debugUtils.SetObjectName(outputImage_->Handle(), "Output Image");
	debugUtils.SetObjectName(outputImageMemory_->Handle(), "Output Image Memory");
	debugUtils.SetObjectName(outputImageView_->Handle(), "Output ImageView");

}

void Application::saveScreenshot(const char* filename, VkCommandBuffer commandBuffer)
{
	//screenshotSaved = false;	
	std::cout << "[Enter Screenshot]" << std::endl;
	bool supportsBlit = true;

	// Check blit support for source and destination
	VkFormatProperties formatProps;

	// Check if the device supports blitting from optimal images (the swapchain images are in optimal format)
	vkGetPhysicalDeviceFormatProperties(Device().PhysicalDevice(), SwapChain().Format(), &formatProps);
	if (!(formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT)) {
		std::cerr << "Device does not support blitting from optimal tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	// Check if the device supports blitting to linear images
	vkGetPhysicalDeviceFormatProperties(Device().PhysicalDevice(), VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
	if (!(formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
		std::cerr << "Device does not support blitting to linear tiled images, using copy instead of blit!" << std::endl;
		supportsBlit = false;
	}

	// Source for the copy is the last rendered swapchain image
	VkImage srcImage = outputImage_->Handle();

	VkImageCreateInfo imageCreateCI;
	imageCreateCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateCI.imageType = VK_IMAGE_TYPE_2D;
	imageCreateCI.pNext = nullptr;
	imageCreateCI.flags = 0;
	imageCreateCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// Note that vkCmdBlitImage (if supported) will also do format conversions if the swapchain color format would differ
	imageCreateCI.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateCI.extent.width = outputImage_->Extent().width;
	imageCreateCI.extent.height = outputImage_->Extent().height;
	imageCreateCI.extent.depth = 1;
	imageCreateCI.arrayLayers = 1;
	imageCreateCI.mipLevels = 1;
	imageCreateCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateCI.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateCI.tiling = VK_IMAGE_TILING_LINEAR;
	imageCreateCI.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	int height = outputImage_->Extent().height;
	int width = outputImage_->Extent().width;

	// Create the image
	VkImage dstImage;
	VkResult(vkCreateImage(Device().Handle(), &imageCreateCI, nullptr, &dstImage));

	VkMemoryRequirements memRequirements;
	//memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	//memAlloc.pNext = nullptr;
	//memAlloc.memoryTypeIndex = 0;
	//VkMemoryAllocateInfo memAllocInfo(memAlloc);
	VkDeviceMemory dstImageMemory;
	vkGetImageMemoryRequirements(Device().Handle(), dstImage, &memRequirements);

	//==
	// Memory must be host visible to copy from
	VkMemoryPropertyFlags properties;
	properties = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VkPhysicalDeviceMemoryProperties memoryProperties;
	memoryProperties = {};
	//memset(&memoryProperties, 0, sizeof(struct VkPhysicalDeviceMemoryProperties));
	vkGetPhysicalDeviceMemoryProperties(Device().PhysicalDevice(), &memoryProperties);

	uint32_t typeBits = memRequirements.memoryTypeBits;
	VkBool32 *memTypeFound = nullptr;
	uint32_t memindex = getMemoryType(typeBits, memoryProperties, properties, memTypeFound);

	VkMemoryAllocateInfo memAllocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memRequirements.size, memindex};
	//==
	VkResult(vkAllocateMemory(Device().Handle(), &memAllocInfo, nullptr, &dstImageMemory));
	VkResult(vkBindImageMemory(Device().Handle(), dstImage, dstImageMemory, 0));


	VkImageMemoryBarrier imageMemoryBarrier{};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.image = dstImage;
	imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &imageMemoryBarrier);

	// Get layout of the image (including row pitch)
	VkImageSubresource subResource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
	VkSubresourceLayout subResourceLayout;
	vkGetImageSubresourceLayout(Device().Handle(), dstImage, &subResource, &subResourceLayout);

	// Map image memory so we can start copying from it
	const char* data;
	VkResult(vkMapMemory(Device().Handle(), dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&data));
	data += subResourceLayout.offset;

	//FILE
	std::ofstream file(filename, std::ios::out | std::ios::binary);

	// ppm header
	file << "P6\n" << width << "\n" << height << "\n" << 255 << "\n";

	bool colorSwizzle = true;

	std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };

	// ppm binary pixel data
	for (uint32_t y = 0; y < height; y++)
	{
		unsigned int* row = (unsigned int*)data;
		for (uint32_t x = 0; x < width; x++)
		{
			
			if (colorSwizzle)
			{
				file.write((char*)row + 2, 1);
				file.write((char*)row + 1, 1);
				file.write((char*)row, 1);
			}
			else
			{
				file.write((char*)row, 3);
			}
			
			row++;
		}
		data += subResourceLayout.rowPitch;
	}
	std::cout << "Screenshot written to disk: " << sizeof(file) << " Bytes" << std::endl;
	file.close();

	std::cout << "Screenshot saved to " << filename << std::endl;

	// Clean up resources
	vkUnmapMemory(Device().Handle(), dstImageMemory);
	vkFreeMemory(Device().Handle(), dstImageMemory, nullptr);
	vkDestroyImage(Device().Handle(), dstImage, nullptr);

	//screenshotSaved = true;
}

uint32_t Application::getMemoryType(uint32_t typeBits, VkPhysicalDeviceMemoryProperties memoryProperties, VkMemoryPropertyFlags properties, VkBool32* memTypeFound) const
{
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memTypeFound)
				{
					*memTypeFound = true;
				}
				return i;
			}
		}
		typeBits >>= 1;
	}

	if (memTypeFound)
	{
		*memTypeFound = false;
		return 0;
	}
	else
	{
		throw std::runtime_error("Could not find a matching memory type");
	}
}


}
