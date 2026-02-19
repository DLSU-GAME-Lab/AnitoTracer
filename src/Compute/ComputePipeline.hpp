#pragma once
#include "Vulkan/Vulkan.hpp"
#include <memory>
#include <vector>
#include <string>

namespace Vulkan
{
	class DescriptorSetManager;
	class Device;
	class Buffer;
}

class ComputePipelineLayout;

class ComputePipeline final
{
public:
	VULKAN_NON_COPIABLE(ComputePipeline)

	ComputePipeline(const Vulkan::Device& device, uint32_t framesInFlight);
	~ComputePipeline();

	const class ComputePipelineLayout& PipelineLayout() const { return *pipelineLayout_; }
	VkDescriptorSet DescriptorSet(const uint32_t index) const;
	size_t CreatePipeline(const std::string& filename);
	VkPipeline GetPipeline(size_t index) const { return pipelines_[index]; }
	void UpdateDescriptorSet(const std::vector<Vulkan::Buffer*>& buffers, uint32_t frameIndex);

private:
	const Vulkan::Device& device_;
	uint32_t framesInFlight_;

	std::vector<VkPipeline> pipelines_;

	std::unique_ptr<Vulkan::DescriptorSetManager> descriptorSetManager_;
	std::unique_ptr<class ComputePipelineLayout> pipelineLayout_;
};