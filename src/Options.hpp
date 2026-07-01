#pragma once

#include <cstdint>
#include <exception>
#include <vector>
#include <vulkan/vulkan.h>

class Options final
{
public:

	class Help : public std::exception
	{
	public:

		Help() = default;
		~Help() = default;
	};

	Options(int argc, const char* argv[], VkPhysicalDevice physicalDevice = VK_NULL_HANDLE);
	~Options() = default;

	/// Recalibrates sample count based on the selected physical device
	/// Call this after device selection if you pass VK_NULL_HANDLE to the constructor
	void RecalibrateSamplesForDevice(VkPhysicalDevice physicalDevice);

	// Application options.
	bool Benchmark{};
	
	// Benchmark options.
	bool BenchmarkNextScenes{};
	uint32_t BenchmarkMaxTime{};

	// Renderer options.
	uint32_t Samples{};
	uint32_t Bounces{};
	uint32_t MaxSamples{};

	// Scene options.
	uint32_t SceneIndex{};

	// Vulkan options
	std::vector<uint32_t> VisibleDevices{};

	// Window options
	uint32_t Width{};
	uint32_t Height{};
	uint32_t PresentMode{};
	bool Fullscreen{};
};
