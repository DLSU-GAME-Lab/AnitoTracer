#pragma once

#include "Vulkan/Vulkan.hpp"
#include <memory>
#include <vector>

#include "Engine/LightSystem/Light.h"

namespace Vulkan
{
	class Buffer;
	class CommandPool;
	class DeviceMemory;
	class Image;
}

namespace Assets
{
	class Model;
	class Texture;
	class TextureImage;

	class RayScene final
	{
	public:

		RayScene(const RayScene&) = delete;
		RayScene(RayScene&&) = delete;
		RayScene& operator = (const RayScene&) = delete;
		RayScene& operator = (RayScene&&) = delete;

		RayScene(Vulkan::CommandPool& commandPool, std::vector<Model>&& models);
		~RayScene();

		const std::vector<Model>& Models() const { return models_; }

		const Vulkan::Buffer& VertexBuffer() const { return *vertexBuffer_; }
		const Vulkan::Buffer& IndexBuffer() const { return *indexBuffer_; }
		const Vulkan::Buffer& OffsetsBuffer() const { return *offsetBuffer_; }

	private:

		const std::vector<Model> models_;

		std::unique_ptr<Vulkan::Buffer> vertexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> vertexBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> indexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> indexBufferMemory_;;

		std::unique_ptr<Vulkan::Buffer> offsetBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> offsetBufferMemory_;
	};

}
