#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

namespace HardwareCheck
{
	/// Determines recommended sample count based on GPU capabilities
	/// Caps at 24 samples for a RTX 5090
	/// Returns a reasonable default for other GPUs
	uint32_t GetRecommendedSampleCount(VkPhysicalDevice physicalDevice);

	/// Gets the GPU name for debugging purposes
	const char* GetGPUName(VkPhysicalDevice physicalDevice);

	/// Evaluates if GPU is high-end (GeForce RTX 50xx or better)
	bool IsHighEndGPU(VkPhysicalDevice physicalDevice);

	/// Gets estimated memory in GB
	uint32_t GetGPUMemoryGB(VkPhysicalDevice physicalDevice);
}
