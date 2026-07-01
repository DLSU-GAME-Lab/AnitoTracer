#pragma once

#include "Vulkan/Vulkan.hpp"
#include <memory>
#include <vector>

namespace Assets
{
	class Scene;
	class RayScene;
	class UniformBuffer;
}

namespace Vulkan
{
	class DescriptorSetManager;
	class ImageView;
	class PipelineLayout;
	class SwapChain;
}

namespace Vulkan::RayTracing
{
	class TopLevelAccelerationStructure;
}

namespace Vulkan::Compute
{
	class ComputeShaderRayTracer final
	{
	public:

		VULKAN_NON_COPIABLE(ComputeShaderRayTracer)

		ComputeShaderRayTracer(
			const Vulkan::SwapChain& swapChain,
			const Vulkan::RayTracing::TopLevelAccelerationStructure& accelerationStructure,
			const Vulkan::ImageView& accumulationImageView,
			const Vulkan::ImageView& outputImageView,
			const Vulkan::ImageView& outputImageViewS,
			const std::vector<Assets::UniformBuffer>& uniformBuffers,
			const Assets::Scene& scene,
			const Assets::RayScene& rayScene);
		~ComputeShaderRayTracer();

		VkDescriptorSet DescriptorSet(uint32_t index) const;
		const Vulkan::PipelineLayout& PipelineLayout() const { return *pipelineLayout_; }

		// Dispatch the compute shader for a given image index
		void Dispatch(VkCommandBuffer commandBuffer, uint32_t imageIndex, const VkExtent2D& extent);

	private:

		const Vulkan::SwapChain& swapChain_;

		VULKAN_HANDLE(VkPipeline, pipeline_)

		std::unique_ptr<Vulkan::DescriptorSetManager> descriptorSetManager_;
		std::unique_ptr<Vulkan::PipelineLayout> pipelineLayout_;
	};

}
