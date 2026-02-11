#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Vulkan
{
	class ImageMemoryBarrier final
	{
	public:

		static void Insert(
			const VkCommandBuffer commandBuffer, 
			const VkImage image, 
			const VkImageSubresourceRange subresourceRange, 
			const VkAccessFlags srcAccessMask,
			const VkAccessFlags dstAccessMask, 
			const VkImageLayout oldLayout, 
			const VkImageLayout newLayout)
		{
			VkImageMemoryBarrier barrier;
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.pNext = nullptr;
			barrier.srcAccessMask = srcAccessMask;
			barrier.dstAccessMask = dstAccessMask;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange = subresourceRange;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1,
				&barrier);
		}
	};

    class BufferMemoryBarrier final
    {
    public:
        struct BufferRange
        {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceSize offset = 0;
            VkDeviceSize size = VK_WHOLE_SIZE;
            uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        };

        static void Insert(
            VkCommandBuffer commandBuffer,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            VkBuffer buffer,
            VkDeviceSize offset = 0,
            VkDeviceSize size = VK_WHOLE_SIZE,
            VkDependencyFlags dependencyFlags = 0,
            uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED)
        {
            BufferRange r{};
            r.buffer = buffer;
            r.offset = offset;
            r.size = size;
            r.srcQueueFamilyIndex = srcQueueFamilyIndex;
            r.dstQueueFamilyIndex = dstQueueFamilyIndex;

            Insert(commandBuffer, srcStageMask, dstStageMask, srcAccessMask, dstAccessMask,
                &r, 1, dependencyFlags);
        }

        // Multi-buffer version
        static void Insert(
            VkCommandBuffer commandBuffer,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            const BufferRange* ranges,
            uint32_t rangeCount,
            VkDependencyFlags dependencyFlags = 0)
        {
            if (commandBuffer == VK_NULL_HANDLE || ranges == nullptr || rangeCount == 0)
                return;

            std::vector<VkBufferMemoryBarrier> barriers;
            barriers.reserve(rangeCount);

            for (uint32_t i = 0; i < rangeCount; ++i)
            {
                const BufferRange& r = ranges[i];
                if (r.buffer == VK_NULL_HANDLE)
                    continue;

                VkBufferMemoryBarrier b{};
                b.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                b.pNext = nullptr;
                b.srcAccessMask = srcAccessMask;
                b.dstAccessMask = dstAccessMask;
                b.srcQueueFamilyIndex = r.srcQueueFamilyIndex;
                b.dstQueueFamilyIndex = r.dstQueueFamilyIndex;
                b.buffer = r.buffer;
                b.offset = r.offset;
                b.size = r.size;

                barriers.push_back(b);
            }

            if (barriers.empty())
                return;

            vkCmdPipelineBarrier(
                commandBuffer,
                srcStageMask,
                dstStageMask,
                dependencyFlags,
                0, nullptr,                                  // memory barriers
                static_cast<uint32_t>(barriers.size()),
                barriers.data(),                              // buffer barriers
                0, nullptr);                                  // image barriers
        }

        // Convenience overload for std::vector
        static void Insert(
            VkCommandBuffer commandBuffer,
            VkPipelineStageFlags srcStageMask,
            VkPipelineStageFlags dstStageMask,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            const std::vector<BufferRange>& ranges,
            VkDependencyFlags dependencyFlags = 0)
        {
            Insert(commandBuffer, srcStageMask, dstStageMask, srcAccessMask, dstAccessMask,
                ranges.data(), static_cast<uint32_t>(ranges.size()), dependencyFlags);
        }
    };

}
