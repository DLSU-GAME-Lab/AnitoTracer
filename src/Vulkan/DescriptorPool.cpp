#include "DescriptorPool.hpp"
#include "Device.hpp"

namespace Vulkan {

DescriptorPool::DescriptorPool(const Vulkan::Device& device, const std::vector<DescriptorBinding>& descriptorBindings, const size_t maxSets) :
	device_(device),
	deviceHandle_(device.Handle())
{
	std::vector<VkDescriptorPoolSize> poolSizes;

	for (const auto& binding : descriptorBindings)
	{
		// Vulkan spec (VUID-VkDescriptorPoolSize-descriptorCount-00302):
		// descriptorCount must be > 0. Skip bindings with 0 descriptors
		// (e.g. texture array when the scene has no textures).
		const uint32_t count = static_cast<uint32_t>(binding.DescriptorCount * maxSets);
		if (count > 0)
		{
			poolSizes.push_back(VkDescriptorPoolSize{ binding.Type, count });
		}
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(maxSets);

	Check(vkCreateDescriptorPool(deviceHandle_, &poolInfo, nullptr, &descriptorPool_),
		"create descriptor pool");
}

DescriptorPool::~DescriptorPool()
{
	if (descriptorPool_ != nullptr && deviceHandle_ != VK_NULL_HANDLE)
	{
		vkDestroyDescriptorPool(deviceHandle_, descriptorPool_, nullptr);
		descriptorPool_ = nullptr;
	}
}

}
