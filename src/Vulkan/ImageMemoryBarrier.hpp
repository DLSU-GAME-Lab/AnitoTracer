#pragma once

#include "Vulkan.hpp"

namespace Vulkan
{
	class ImageMemoryBarrier final
	{
	public:

		static void Insert(
			const VkCommandBuffer commandBuffer,
			const VkImage image,
			const VkImageSubresourceRange& subresourceRange,
			const VkAccessFlags srcAccessMask,
			const VkAccessFlags dstAccessMask,
			const VkImageLayout oldLayout,
			const VkImageLayout newLayout,
			const VkAccessFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			const VkAccessFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
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

			vkCmdPipelineBarrier(
				commandBuffer,
				srcStageMask,
				dstStageMask,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&barrier);
		}
	};

}
