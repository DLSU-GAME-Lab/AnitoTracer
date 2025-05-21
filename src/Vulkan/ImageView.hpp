#pragma once

#include "Vulkan.hpp"

namespace Vulkan
{
	class Device;

	class ImageView final
	{
	public:

		VULKAN_NON_COPIABLE(ImageView)

		explicit ImageView(const Device& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, uint32_t layerCount = 1);
		~ImageView();

		const class Device& Device() const { return device_; }

	private:

		const class Device& device_;
		const VkImage image_;
		const VkFormat format_;

		VULKAN_HANDLE(VkImageView, imageView_)
	};

}
