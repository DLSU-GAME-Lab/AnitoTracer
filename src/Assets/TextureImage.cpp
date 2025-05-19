#include "TextureImage.hpp"
#include "Texture.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/Sampler.hpp"
#include <cstring>
#include <stdexcept>

namespace Assets {

TextureImage::TextureImage(Vulkan::CommandPool& commandPool, const Texture& texture)
{
	// Create a host staging buffer and copy the image into it.
	const VkDeviceSize imageSize = texture.Width() * texture.Height() * 4;
	const auto& device = commandPool.Device();

	auto stagingBuffer = std::make_unique<Vulkan::Buffer>(device, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	auto stagingBufferMemory = stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	const auto data = stagingBufferMemory.Map(0, imageSize);
	std::memcpy(data, texture.Pixels(), imageSize);
	stagingBufferMemory.Unmap();

	// Create the device side image, memory, view and sampler.
	image_.reset(new Vulkan::Image(device, VkExtent2D{ static_cast<uint32_t>(texture.Width()), static_cast<uint32_t>(texture.Height()) }, VK_FORMAT_R8G8B8A8_UNORM));
	imageMemory_.reset(new Vulkan::DeviceMemory(image_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
	imageView_.reset(new Vulkan::ImageView(device, image_->Handle(), image_->Format(), VK_IMAGE_ASPECT_COLOR_BIT));
	sampler_.reset(new Vulkan::Sampler(device, Vulkan::SamplerConfig()));

	// Transfer the data to device side.
	image_->TransitionImageLayout(commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	image_->CopyFrom(commandPool, *stagingBuffer);
	image_->TransitionImageLayout(commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Delete the buffer before the memory
	stagingBuffer.reset();
}

TextureImage::TextureImage(Vulkan::CommandPool& commandPool, const Assets::CubeMapTexture& cubemap) 
{
	const auto& device = commandPool.Device();
	int width = 0, height = 0, channels = 0;
	const int faceCount = 6;

	const VkDeviceSize faceSize = [&]() {
		unsigned char* pixels = stbi_load(cubemap.faces[0].c_str(), &width, &height, &channels, STBI_rgb_alpha);
		if (!pixels) throw std::runtime_error("Failed to load cubemap face: " + cubemap.faces[0]);
		VkDeviceSize size = width * height * 4;
		stbi_image_free(pixels);
		return size;
		}();

	const VkDeviceSize totalSize = faceSize * faceCount;

	auto stagingBuffer = std::make_unique<Vulkan::Buffer>(device, totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	auto stagingMemory = stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* mapped = stagingMemory.Map(0, totalSize);

	for (int i = 0; i < faceCount; ++i)
	{
		int w, h, c;
		unsigned char* pixels = stbi_load(cubemap.faces[i].c_str(), &w, &h, &c, STBI_rgb_alpha);
		if (!pixels) throw std::runtime_error("Failed to load cubemap face: " + cubemap.faces[i]);

		if (w != width || h != height)
		{
			throw std::runtime_error("Cubemap face size mismatch: " + cubemap.faces[i]);
		}

		std::memcpy(static_cast<char*>(mapped) + i * faceSize, pixels, faceSize);
		stbi_image_free(pixels);
	}

	stagingMemory.Unmap();

	image_.reset(new Vulkan::Image(
		device,
		VkExtent2D{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) },
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		6, 
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	));

	imageMemory_.reset(new Vulkan::DeviceMemory(image_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));

	image_->TransitionImageLayout(commandPool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6);
	image_->CopyFrom(commandPool, *stagingBuffer, 6, faceSize);
	image_->TransitionImageLayout(commandPool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6);

	imageView_.reset(new Vulkan::ImageView(
		device,
		image_->Handle(),
		image_->Format(),
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		6
	));

	sampler_.reset(new Vulkan::Sampler(device, Vulkan::SamplerConfig()));

	stagingBuffer.reset(); 
}

TextureImage::~TextureImage()
{
	sampler_.reset();
	imageView_.reset();
	image_.reset();
	imageMemory_.reset();
}

}
