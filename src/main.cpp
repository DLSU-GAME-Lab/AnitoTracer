
#include "Vulkan/Enumerate.hpp"
#include "Vulkan/Strings.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/Version.hpp"
#include "Utilities/Console.hpp"
#include "Utilities/Exception.hpp"
#include "Utilities/HardwareCheck.hpp"
#include "Options.hpp"
#include "RayTracer.hpp"
#include "Engine/Physics/PhysicsEngine.hpp"
#include "Engine/Physics/PhysicsWorld.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace
{
	UserSettings CreateUserSettings(const Options& options);
	Anito::Physics::PhysicsWorldPtr InitializePhysics();
	void PrintVulkanSdkInformation();
	void PrintVulkanInstanceInformation(const Vulkan::Application& application, bool benchmark);
	void PrintVulkanLayersInformation(const Vulkan::Application& application, bool benchmark);
	void PrintVulkanDevices(const Vulkan::Application& application, const std::vector<uint32_t>& visible_devices);
	void PrintVulkanSwapChainInformation(const Vulkan::Application& application, bool benchmark);
	void SetVulkanDevice(Vulkan::Application& application, const std::vector<uint32_t>& visible_devices, Options& options);
}

extern "C"
{
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
	__declspec(dllexport) unsigned int NvOptimusEnablement = 0x00000001;
}

int main(int argc, const char* argv[]) noexcept
{
	try
	{
		Options options(argc, argv);
		const Vulkan::WindowConfig windowConfig
		{
			"DLSU-IET Engine - Interactive Ray Tracer",
			options.Width,
			options.Height,
			options.Benchmark && options.Fullscreen,
			options.Fullscreen,
			!options.Fullscreen
		};

		// Create initial settings with default sample count
		UserSettings userSettings = CreateUserSettings(options);
		RayTracer::initialize(userSettings, windowConfig, static_cast<VkPresentModeKHR>(options.PresentMode));
		RayTracer* application = RayTracer::getInstance();

		Anito::Physics::PhysicsWorldPtr defaultPhysicsWorld = InitializePhysics();

		PrintVulkanSdkInformation();
		PrintVulkanInstanceInformation(*application, options.Benchmark);
		PrintVulkanLayersInformation(*application, options.Benchmark);
		PrintVulkanDevices(*application, options.VisibleDevices);

		// Detect device and recalibrate options
		SetVulkanDevice(*application, options.VisibleDevices, options);

		// Update NumberOfSamples based on hardware (now that we have the device info)
		application->getUserSettings().NumberOfSamples = options.Samples;

		PrintVulkanSwapChainInformation(*application, options.Benchmark);

		application->Run();

		return EXIT_SUCCESS;
	}

	catch (const Options::Help&)
	{
		return EXIT_SUCCESS;
	}

	catch (const std::exception& exception)
	{
		Utilities::Console::Write(Utilities::Severity::Fatal, [&exception]() 
		{
			const auto stacktrace = boost::get_error_info<traced>(exception);

			std::cerr << "FATAL: " << exception.what() << std::endl;

			if (stacktrace)
			{
				std::cerr << '\n' << *stacktrace << '\n';
			}
		});
	}

	catch (...)
	{
		Utilities::Console::Write(Utilities::Severity::Fatal, []()
		{
			std::cerr << "FATAL: caught unhandled exception" << std::endl;
		});
	}

	return EXIT_FAILURE;
}

namespace
{

	UserSettings CreateUserSettings(const Options& options)
	{
		UserSettings userSettings{};

		userSettings.Benchmark = options.Benchmark;
		userSettings.BenchmarkNextScenes = options.BenchmarkNextScenes;
		userSettings.BenchmarkMaxTime = options.BenchmarkMaxTime;
		
		userSettings.SceneIndex = options.SceneIndex;

		userSettings.IsRayTraced = true;
		userSettings.AccumulateRays = true;
		userSettings.MultiSampling = false;
		userSettings.aaValue = 2;
		userSettings.NumberOfSamples = options.Samples;
		userSettings.NumberOfBounces = options.Bounces;
		userSettings.MaxNumberOfSamples = options.MaxSamples;

		userSettings.ShowSettings = !options.Benchmark;
		userSettings.ShowOverlay = false;

		userSettings.ShowHeatmap = false;
		userSettings.HeatmapScale = 1.5f;

		return userSettings;
	}

	Anito::Physics::PhysicsWorldPtr InitializePhysics() {
		Anito::Physics::PhysicsWorldPtr mDefaultPhysicsWorld;

		try {
			auto& engine = Anito::Physics::PhysicsEngine::Get();

			if (!engine.IsInitialized()) {
				if (engine.Initialize()) {
					mDefaultPhysicsWorld = engine.GetDefaultWorld();
					std::cout << "[RayTracer] Physics engine initialized successfully" << std::endl;
				}
				else {
					std::cerr << "[RayTracer] Failed to initialize physics engine, continuing without physics" << std::endl;
					mDefaultPhysicsWorld = nullptr;
				}
			}
		}
		catch (const std::exception& e) {
			std::cerr << "[RayTracer] Exception during physics engine initialization: " << e.what() << std::endl;
			mDefaultPhysicsWorld = nullptr;
		}

		return mDefaultPhysicsWorld;
	}

	void PrintVulkanSdkInformation()
	{
		std::cout << "Vulkan SDK Header Version: " << VK_HEADER_VERSION << std::endl;
		std::cout << std::endl;
	}
	
	void PrintVulkanInstanceInformation(const Vulkan::Application& application, const bool benchmark)
	{
		if (benchmark)
		{
			return;
		}

		std::cout << "Vulkan Instance Extensions: " << std::endl;

		for (const auto& extension : application.Extensions())
		{
			std::cout << "- " << extension.extensionName << " (" << Vulkan::Version(extension.specVersion) << ")" << std::endl;
		}

		std::cout << std::endl;
	}

	void PrintVulkanLayersInformation(const Vulkan::Application& application, const bool benchmark)
	{
		if (benchmark)
		{
			return;
		}

		std::cout << "Vulkan Instance Layers: " << std::endl;

		for (const auto& layer : application.Layers())
		{
			std::cout
				<< "- " << layer.layerName
				<< " (" << Vulkan::Version(layer.specVersion) << ")"
				<< " : " << layer.description << std::endl;
		}

		std::cout << std::endl;
	}
	
	void PrintVulkanDevices(const Vulkan::Application& application, const std::vector<uint32_t>& visible_devices)
	{
		std::cout << "Vulkan Devices: " << std::endl;

		for (const auto& device : application.PhysicalDevices())
		{
			VkPhysicalDeviceDriverProperties driverProp{};
			driverProp.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
			
			VkPhysicalDeviceProperties2 deviceProp{};
			deviceProp.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			deviceProp.pNext = &driverProp;
			
			vkGetPhysicalDeviceProperties2(device, &deviceProp);

			VkPhysicalDeviceFeatures features;
			vkGetPhysicalDeviceFeatures(device, &features);

			const auto& prop = deviceProp.properties;

			// Check whether device has been explicitly filtered out.
			if (!visible_devices.empty() && std::find(visible_devices.begin(), visible_devices.end(), prop.deviceID) == visible_devices.end())
			{
				break;
			}

			const Vulkan::Version vulkanVersion(prop.apiVersion);
			const Vulkan::Version driverVersion(prop.driverVersion, prop.vendorID);

			std::cout << "- [" << prop.deviceID << "] ";
			std::cout << Vulkan::Strings::VendorId(prop.vendorID) << " '" << prop.deviceName;
			std::cout << "' (";
			std::cout << Vulkan::Strings::DeviceType(prop.deviceType) << ": ";
			std::cout << "vulkan " << vulkanVersion << ", ";
			std::cout << "driver " << driverProp.driverName << " " << driverProp.driverInfo << " - " << driverVersion;
			std::cout << ")" << std::endl;
		}

		std::cout << std::endl;
	}

	void PrintVulkanSwapChainInformation(const Vulkan::Application& application, const bool benchmark)
	{
		const auto& swapChain = application.SwapChain();

		std::cout << "Swap Chain: " << std::endl;
		std::cout << "- image count: " << swapChain.Images().size() << std::endl;
		std::cout << "- present mode: " << swapChain.PresentMode() << std::endl;
		std::cout << std::endl;
	}

	void SetVulkanDevice(Vulkan::Application& application, const std::vector<uint32_t>& visible_devices, Options& options)
	{
		const auto& physicalDevices = application.PhysicalDevices();
		const auto result = std::find_if(physicalDevices.begin(), physicalDevices.end(), [&](const VkPhysicalDevice& device)
		{
			VkPhysicalDeviceProperties2 prop{};
			prop.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			vkGetPhysicalDeviceProperties2(device, &prop);

			// Check whether device has been explicitly filtered out.
			if (!visible_devices.empty() && std::find(visible_devices.begin(), visible_devices.end(), prop.properties.deviceID) == visible_devices.end())
			{
				return false;
			}

			// We want a device with geometry shader support.
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

			if (!deviceFeatures.geometryShader)
			{
				return false;
			}

			// We want a device that supports the ray tracing extension.
			const auto extensions = Vulkan::GetEnumerateVector(device, static_cast<const char*>(nullptr), vkEnumerateDeviceExtensionProperties);
			const auto hasRayTracing = std::any_of(extensions.begin(), extensions.end(), [](const VkExtensionProperties& extension)
			{
				return strcmp(extension.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0;
			});

			if (!hasRayTracing)
			{
				return false;
			}

			// We want a device with a graphics queue.
			const auto queueFamilies = Vulkan::GetEnumerateVector(device, vkGetPhysicalDeviceQueueFamilyProperties);
			const auto hasGraphicsQueue = std::any_of(queueFamilies.begin(), queueFamilies.end(), [](const VkQueueFamilyProperties& queueFamily)
			{
				return queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT;
			});

			return hasGraphicsQueue;
		});

		if (result == physicalDevices.end())
		{
			Throw(std::runtime_error("cannot find a suitable device"));
		}

		VkPhysicalDeviceProperties2 deviceProp{};
		deviceProp.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		vkGetPhysicalDeviceProperties2(*result, &deviceProp);

		std::cout << "Setting Device [" << deviceProp.properties.deviceID << "]:" << std::endl;
		std::cout << "- GPU Name: " << HardwareCheck::GetGPUName(*result) << std::endl;
		std::cout << "- GPU Memory: " << HardwareCheck::GetGPUMemoryGB(*result) << " GB" << std::endl;
		std::cout << "- High-End GPU: " << (HardwareCheck::IsHighEndGPU(*result) ? "Yes" : "No") << std::endl;

		// Recalibrate samples based on the identified device
		options.RecalibrateSamplesForDevice(*result);
		std::cout << "- Recommended Samples: " << options.Samples << std::endl;

		application.SetPhysicalDevice(*result);

		std::cout << std::endl;
	}

}
