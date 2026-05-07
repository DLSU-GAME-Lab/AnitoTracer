#include "HardwareCheck.hpp"
#include <string>

namespace HardwareCheck
{
	uint32_t GetRecommendedSampleCount(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		// Calculate total GPU memory
		VkDeviceSize totalMemory = 0;
		for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i)
		{
			if (memProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				totalMemory = memProperties.memoryHeaps[i].size;
				break;
			}
		}

		const uint32_t memoryGB = static_cast<uint32_t>(totalMemory / (1024ULL * 1024ULL * 1024ULL));
		const std::string deviceName(properties.deviceName);

		// Ultra-high-end cards (cap at 24)
		// NVIDIA: RTX 5090, Titan
		// AMD: MI325X, MI300X, Radeon Pro W7900 XTX
		if (deviceName.find("5090") != std::string::npos ||
			deviceName.find("Titan") != std::string::npos ||
			deviceName.find("MI325") != std::string::npos ||
			deviceName.find("MI300X") != std::string::npos ||
			deviceName.find("W7900") != std::string::npos)
		{
			return 24;
		}

		// High-end cards (20 samples)
		// NVIDIA: RTX 4090, 5080, 4080, A100
		// AMD: MI300, MI250X, MI250, Radeon Pro W7800
		if (deviceName.find("4090") != std::string::npos ||
			deviceName.find("5080") != std::string::npos ||
			deviceName.find("4080") != std::string::npos ||
			deviceName.find("A100") != std::string::npos ||
			deviceName.find("MI300") != std::string::npos ||
			deviceName.find("MI250") != std::string::npos ||
			deviceName.find("W7800") != std::string::npos)
		{
			return 20;
		}

		// Mid-high-end cards (16 samples)
		// NVIDIA: RTX 4070 Ti, 5070 Ti, 3080
		// AMD: MI100, Radeon Pro W7700, RX 7900 XTX, RX 6900 XT
		if (deviceName.find("4070") != std::string::npos ||
			deviceName.find("5070") != std::string::npos ||
			deviceName.find("3080") != std::string::npos ||
			deviceName.find("MI100") != std::string::npos ||
			deviceName.find("W7700") != std::string::npos ||
			deviceName.find("7900") != std::string::npos ||
			deviceName.find("6900") != std::string::npos ||
			memoryGB >= 16)
		{
			return 16;
		}

		// Mid-range cards (12 samples)
		// NVIDIA: RTX 4060 Ti, 3070, 3060
		// AMD: Radeon Pro W6800, RX 7800 XT, RX 6800 XT, RX 5700 XT
		if (deviceName.find("4060") != std::string::npos ||
			deviceName.find("3070") != std::string::npos ||
			deviceName.find("3060") != std::string::npos ||
			deviceName.find("W6800") != std::string::npos ||
			deviceName.find("7800") != std::string::npos ||
			deviceName.find("6800") != std::string::npos ||
			deviceName.find("5700") != std::string::npos ||
			memoryGB >= 12)
		{
			return 12;
		}

		// Entry-level cards or integrated graphics (8 samples)
		// NVIDIA: Entry cards, Tegra
		// AMD: Integrated Radeon, Mobile Radeon, older cards
		if (deviceName.find("Integrated") != std::string::npos ||
			deviceName.find("Intel") != std::string::npos ||
			deviceName.find("Radeon") != std::string::npos && memoryGB < 8 ||
			memoryGB < 4)
		{
			return 8;
		}

		// Default conservative estimate for unknown cards based on memory
		if (memoryGB >= 8)
		{
			return 12;
		}

		return 8;
	}

	const char* GetGPUName(VkPhysicalDevice physicalDevice)
	{
		static VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		return properties.deviceName;
	}

	bool IsHighEndGPU(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		const std::string deviceName(properties.deviceName);

		// NVIDIA high-end
		if (deviceName.find("5090") != std::string::npos ||
			deviceName.find("5080") != std::string::npos ||
			deviceName.find("4090") != std::string::npos ||
			deviceName.find("4080") != std::string::npos ||
			deviceName.find("Titan") != std::string::npos ||
			deviceName.find("A100") != std::string::npos)
		{
			return true;
		}

		// AMD high-end
		if (deviceName.find("MI325") != std::string::npos ||
			deviceName.find("MI300") != std::string::npos ||
			deviceName.find("MI250") != std::string::npos ||
			deviceName.find("W7900") != std::string::npos ||
			deviceName.find("W7800") != std::string::npos ||
			deviceName.find("7900") != std::string::npos ||
			deviceName.find("6900") != std::string::npos)
		{
			return true;
		}

		return false;
	}

	uint32_t GetGPUMemoryGB(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		VkDeviceSize totalMemory = 0;
		for (uint32_t i = 0; i < memProperties.memoryHeapCount; ++i)
		{
			if (memProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				totalMemory = memProperties.memoryHeaps[i].size;
				break;
			}
		}

		return static_cast<uint32_t>(totalMemory / (1024ULL * 1024ULL * 1024ULL));
	}
}
