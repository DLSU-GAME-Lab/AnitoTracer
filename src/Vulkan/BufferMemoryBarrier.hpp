#pragma once

#include "Vulkan.hpp"

namespace Vulkan
{
	class BufferMemoryBarrier final
	{
	public:

		static void Insert(
			const VkCommandBuffer commandBuffer,
			const VkBuffer buffer,
			const VkDeviceSize size,
			const VkDeviceSize offset,
			const VkAccessFlags srcAccessMask,
			const VkAccessFlags dstAccessMask)
		{
			VkBufferMemoryBarrier barrier;
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.pNext = nullptr;
			barrier.srcAccessMask = srcAccessMask;
			barrier.dstAccessMask = dstAccessMask;
			barrier.size = size;
			barrier.offset = offset;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.buffer = buffer;
			
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 1, &barrier, 0,
				nullptr);
		}
	};

}
