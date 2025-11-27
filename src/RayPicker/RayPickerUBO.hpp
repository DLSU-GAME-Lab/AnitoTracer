#pragma once
#include <glm/glm.hpp>
#include <memory>

namespace Vulkan
{
	class Buffer;
	class Device;
	class DeviceMemory;
}

class RayPickerUBO
{
public:
	glm::mat4 ModelView;
	glm::mat4 Projection;
	glm::mat4 ModelViewInverse;
	glm::mat4 ProjectionInverse;
};

class PushConstantScreenPosition
{
public:
	glm::vec4 origin;
	glm::vec4 dir;
};

class RayPickerUniformBuffer
{
public:
	RayPickerUniformBuffer(const RayPickerUniformBuffer&) = delete;
	RayPickerUniformBuffer& operator = (const RayPickerUniformBuffer&) = delete;
	RayPickerUniformBuffer& operator = (RayPickerUniformBuffer&&) = delete;

	explicit RayPickerUniformBuffer(const Vulkan::Device& device, const size_t bufferSize);
	RayPickerUniformBuffer(RayPickerUniformBuffer&& other) noexcept;
	~RayPickerUniformBuffer();

	const Vulkan::Buffer& Buffer() const { return *buffer_; }

	void SetValue(const RayPickerUBO& ubo);
	void SetValue(const PushConstantScreenPosition& ubo);

private:

	std::unique_ptr<Vulkan::Buffer> buffer_;
	std::unique_ptr<Vulkan::DeviceMemory> memory_;
};