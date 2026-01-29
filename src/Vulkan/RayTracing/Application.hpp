#pragma once

#include "Vulkan/Application.hpp"
#include "RayTracingProperties.hpp" 
#include "Compute/WorkLoaderPipeline.hpp"
#include "Compute/PixelManagementPipeline.hpp"

namespace Vulkan
{
	class CommandBuffers;
	class Buffer;
	class DeviceMemory;
	class Image;
	class ImageView;
}

namespace Vulkan::RayTracing
{
	class Application : public Vulkan::Application
	{
	public:

		VULKAN_NON_COPIABLE(Application);

	protected:

		Application(const WindowConfig& windowConfig, VkPresentModeKHR presentMode, bool enableValidationLayers);
		~Application();

		void SetPhysicalDevice(VkPhysicalDevice physicalDevice,
			std::vector<const char*>& requiredExtensions,
			VkPhysicalDeviceFeatures& deviceFeatures,
			void* nextDeviceFeatures) override;
		
		void OnDeviceSet() override;
		void CreateAccelerationStructures();
		void UpdateAccelerationStructures();
		void DeleteAccelerationStructures();
		void CreateSwapChain() override;
		void DeleteSwapChain() override;
		void Render(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
		void Compute(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
			   
	protected:

		void CreateBottomLevelStructures(VkCommandBuffer commandBuffer);
		void CreateTopLevelStructures(VkCommandBuffer commandBuffer);
		void UpdateTopLevelStructures(VkCommandBuffer commandBuffer);
		void CreateOutputImage();
		void CreatePixelManagementBuffers();

		std::unique_ptr<class DeviceProcedures> deviceProcedures_;
		std::unique_ptr<class RayTracingProperties> rayTracingProperties_;

		std::vector<class BottomLevelAccelerationStructure> bottomAs_;
		std::unique_ptr<Buffer> bottomBuffer_;
		std::unique_ptr<DeviceMemory> bottomBufferMemory_;
		std::unique_ptr<Buffer> bottomScratchBuffer_;
		std::unique_ptr<DeviceMemory> bottomScratchBufferMemory_;
		std::vector<class TopLevelAccelerationStructure> topAs_;
		std::unique_ptr<Buffer> topBuffer_;
		std::unique_ptr<DeviceMemory> topBufferMemory_;
		std::unique_ptr<Buffer> topScratchBuffer_;
		std::unique_ptr<DeviceMemory> topScratchBufferMemory_;
		std::unique_ptr<Buffer> instancesBuffer_;
		std::unique_ptr<DeviceMemory> instancesBufferMemory_;

		std::unique_ptr<Image> accumulationImage_;
		std::unique_ptr<DeviceMemory> accumulationImageMemory_;
		std::unique_ptr<ImageView> accumulationImageView_;

		std::unique_ptr<Image> outputImage_;
		std::unique_ptr<DeviceMemory> outputImageMemory_;
		std::unique_ptr<ImageView> outputImageView_;
		
		std::unique_ptr<class RayTracingPipeline> rayTracingPipeline_;
		std::unique_ptr<class ShaderBindingTable> shaderBindingTable_;

		std::unique_ptr<class PixelManagementPipeline> pixelManagementPipeline_;
		std::unique_ptr<class WorkLoaderPipeline> workLoaderPipeline_;

		/* Buffers for Pixel State Management */

		std::unique_ptr<Buffer> dirtyObjectBoundsBuffer_;
		std::unique_ptr<DeviceMemory> dirtyObjectBoundsBufferMemory_;

		std::unique_ptr<Buffer> dirtyObjectCountBuffer_;
		std::unique_ptr<DeviceMemory> dirtyObjectCountBufferMemory_;

		std::unique_ptr<Buffer> cleanStatusBuffer_;
		std::unique_ptr<DeviceMemory> cleanStatusBufferMemory_;

		std::unique_ptr<Buffer> rayCountBuffer_;
		std::unique_ptr<DeviceMemory> rayCountBufferMemory_;

		std::unique_ptr<Buffer> pixelWeightBuffer_;
		std::unique_ptr<DeviceMemory> pixelWeightBufferMemory_;

		std::unique_ptr<Buffer> workQueueBuffer_;
		std::unique_ptr<DeviceMemory> workQueueBufferMemory_;

		std::unique_ptr<Buffer> workQueueCountBuffer_;
		std::unique_ptr<DeviceMemory> workQueueCountBufferMemory_;
	};

}
