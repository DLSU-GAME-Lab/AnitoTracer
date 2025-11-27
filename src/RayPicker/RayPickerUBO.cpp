#include "RayPickerUBO.hpp"
#include "Vulkan/Buffer.hpp"
#include <cstring>

RayPickerUniformBuffer::RayPickerUniformBuffer(const Vulkan::Device& device, const size_t bufferSize)
{
	buffer_.reset(new Vulkan::Buffer(device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT));
	memory_.reset(new Vulkan::DeviceMemory(buffer_->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)));
}

RayPickerUniformBuffer::RayPickerUniformBuffer(RayPickerUniformBuffer&& other) noexcept :
	buffer_(other.buffer_.release()),
	memory_(other.memory_.release())
{
}

RayPickerUniformBuffer::~RayPickerUniformBuffer()
{
	buffer_.reset();
	memory_.reset(); // release memory after bound buffer has been destroyed
}

void RayPickerUniformBuffer::SetValue(const RayPickerUBO& ubo)
{
	const auto data = memory_->Map(0, sizeof(RayPickerUBO));
	std::memcpy(data, &ubo, sizeof(ubo));
	memory_->Unmap();
}

void RayPickerUniformBuffer::SetValue(const PushConstantScreenPosition& ubo)
{
	const auto data = memory_->Map(0, sizeof(PushConstantScreenPosition));
	std::memcpy(data, &ubo, sizeof(ubo));
	memory_->Unmap();
}
