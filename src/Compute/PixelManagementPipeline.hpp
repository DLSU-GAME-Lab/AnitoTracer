#pragma once
#include "Vulkan/Vulkan.hpp"
#include <memory>

namespace Vulkan
{
	class DescriptorSetManager;
	class SwapChain;
}

class ComputePipelineLayout;

class PixelManagementPipeline final
{
public:

	VULKAN_NON_COPIABLE(PixelManagementPipeline)

	PixelManagementPipeline(const Vulkan::SwapChain& swapChain,
			const VkBuffer dirtyObjectsBoundsBuffer,
			const VkBuffer dirtyObjectsCountBuffer,
			const VkBuffer pixelCleanStatusBuffer,
			const VkBuffer RayCountBuffer,
			const VkBuffer PixelWeightBuffer);
	~PixelManagementPipeline();

	const class ComputePipelineLayout& PipelineLayout() const { return *pipelineLayout_; }

	VkDescriptorSet DescriptorSet(const uint32_t index) const;

private:

	const Vulkan::SwapChain& swapChain_;

	VULKAN_HANDLE(VkPipeline, pipeline_)

	std::unique_ptr<Vulkan::DescriptorSetManager> descriptorSetManager_;
	std::unique_ptr<class ComputePipelineLayout> pipelineLayout_;
};