#pragma once
#include "RayPickerPipeline.hpp"
#include "Vulkan/CommandBuffers.hpp"
#include "Vulkan/RayTracing/ShaderBindingTable.hpp"
#include "Vulkan/Buffer.hpp"
#include <glm/glm.hpp>

namespace Vulkan::RayTracing
{
	class DeviceProcedures;
	class TopLevelAccelerationStructure;
	class RayTracingProperties;
}

class Vulkan::SwapChain;
class RayPickerUniformBuffer;

class RayPicker
{
public:
	struct Result
	{
		int objectID;
		int instanceID;
		int primID;
		float hitT;
		float bary0, bary1;
		bool hit;
	};

	RayPicker(const Vulkan::RayTracing::DeviceProcedures& dp, const Vulkan::SwapChain& swapChain, Vulkan::CommandPool& cmdPool, const Vulkan::RayTracing::TopLevelAccelerationStructure& accelerationStructure,
		const std::vector<RayPickerUniformBuffer>& uniformBuffers, const Assets::Scene& scene, const Vulkan::RayTracing::RayTracingProperties& properties);
	~RayPicker();

	Result pick(const Vulkan::RayTracing::DeviceProcedures& dp, const Vulkan::Device& device, glm::vec3 origin, glm::vec3 dir, const uint32_t imageIndex);

private:
	std::unique_ptr<Vulkan::Buffer> resultBuffer;
	std::unique_ptr<Vulkan::Buffer> stagingBuffer;

	std::unique_ptr<Vulkan::DeviceMemory> resultMemory;
	std::unique_ptr<Vulkan::DeviceMemory> stagingMemory;

	std::unique_ptr<class Vulkan::CommandBuffers> commandBuffers;
	std::unique_ptr<class Vulkan::RayTracing::RayPickerPipeline> rayPickerPipeline;
	std::unique_ptr<class Vulkan::RayTracing::ShaderBindingTable> rayPickerSBT;
};