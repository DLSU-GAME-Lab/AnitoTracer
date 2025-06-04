#pragma once

#include "Vulkan/Vulkan.hpp"
#include <memory>
#include <vector>

#include "Engine/LightSystem/Light.h"
#include "Ray.hpp"

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
	class Ray;
	class RayScene final
	{
	public:

		RayScene(const RayScene&) = delete;
		RayScene(RayScene&&) = delete;
		RayScene& operator = (const RayScene&) = delete;
		RayScene& operator = (RayScene&&) = delete;

		RayScene(Vulkan::CommandPool& commandPool, std::vector<Model>&& models);
		~RayScene();

		void Update();

		const std::vector<Model>& Models() const { return models_; }
		const std::vector<Ray*>& Rays() const { return rays_; }
		const Vulkan::Buffer& RayDebugBuffer() const { return *rayDebugBuffer_; }
		/*const Vulkan::Buffer& VertexBuffer() const { return *vertexBuffer_; }
		const Vulkan::Buffer& IndexBuffer() const { return *indexBuffer_; }
		const Vulkan::Buffer& OffsetsBuffer() const { return *offsetBuffer_; }*/

	private:

		const std::vector<Model> models_;
		std::vector<Ray*> rays_;

		std::unique_ptr<Vulkan::Buffer> rayDebugBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> rayDebugBufferMemory_;

		/*std::unique_ptr<Vulkan::Buffer> vertexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> vertexBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> indexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> indexBufferMemory_;;

		std::unique_ptr<Vulkan::Buffer> offsetBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> offsetBufferMemory_;*/
	};

}
