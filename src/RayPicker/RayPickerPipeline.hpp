#pragma once
#include "Vulkan/Vulkan.hpp"
#include "RayPickerPipelineLayout.hpp"
#include <memory>
#include <vector>

namespace Assets
{
	class Scene;
}

namespace Vulkan
{
	class DescriptorSetManager;
	class Buffer;
	class SwapChain;
}

class RayPickerUniformBuffer;

namespace Vulkan::RayTracing
{
	class DeviceProcedures;
	class TopLevelAccelerationStructure;

	class RayPickerPipeline final
	{
	public:
		VULKAN_NON_COPIABLE(RayPickerPipeline)

			RayPickerPipeline(
				const DeviceProcedures& deviceProcedures,
				const SwapChain& swapChain,
				const TopLevelAccelerationStructure& accelerationStructure,
				const Buffer& resultBuffer,
				const std::vector<RayPickerUniformBuffer>& uniformBuffers,
				const Assets::Scene& scene);
		~RayPickerPipeline();
		
		uint32_t RayGenShaderIndex() const { return rayGenIndex_; }
		uint32_t MissShaderIndex() const { return missIndex_; }
		uint32_t HitGroupIndex() const { return hitGroupIndex_; }

		VkDescriptorSet DescriptorSet(uint32_t index) const;
		const class RayPickerPipelineLayout& PipelineLayout() const { return *pipelineLayout_; }

	private:
		const SwapChain& swapChain_;

		VULKAN_HANDLE(VkPipeline, pipeline_)

		std::unique_ptr<DescriptorSetManager> descriptorSetManager_;
		std::unique_ptr<class RayPickerPipelineLayout> pipelineLayout_;

		uint32_t rayGenIndex_;
		uint32_t missIndex_;
		uint32_t hitGroupIndex_;
	};
}