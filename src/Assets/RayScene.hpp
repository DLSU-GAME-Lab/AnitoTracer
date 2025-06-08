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
	class Ray;
	class Scene;
	class RayScene final
	{
	public:

		RayScene(const RayScene&) = delete;
		RayScene(RayScene&&) = delete;
		RayScene& operator = (const RayScene&) = delete;
		RayScene& operator = (RayScene&&) = delete;

		RayScene(Vulkan::CommandPool& commandPool);
		~RayScene();

		void Update(Vulkan::CommandPool& commandPool);

		const std::vector<Model>& Models() const { return models_; }
		const std::vector<Ray*>& Rays() const { return rays_; }

		const Vulkan::Buffer& RayVertexBuffer() const { return *rayVertexBuffer_; }
		const Vulkan::Buffer& RayCounterBuffer() const { return *rayCounterBuffer_; }

		/*const Vulkan::Buffer& VertexBuffer() const { return *vertexBuffer_; }
		const Vulkan::Buffer& IndexBuffer() const { return *indexBuffer_; }
		const Vulkan::Buffer& OffsetsBuffer() const { return *offsetBuffer_; }*/

	private:
		uint32_t GetRayCounter(Vulkan::CommandPool& commandPool);

		const uint32_t maxRays_ = 16;

		const std::vector<Model> models_;
		std::vector<Ray*> rays_;

		std::unique_ptr<Vulkan::Buffer> rayVertexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> rayVertexBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> rayCounterBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> rayCounterBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> rayIndexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> rayIndexBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> rayInfoBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> rayInfoBufferMemory_;

		bool alreadyPut_ = false;
		//std::unique_ptr<Vulkan::Buffer> maxRaysBuffer_;
		//std::unique_ptr<Vulkan::DeviceMemory> maxRaysBufferMemory_;

		/*std::unique_ptr<Vulkan::Buffer> vertexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> vertexBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> indexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> indexBufferMemory_;;

		std::unique_ptr<Vulkan::Buffer> offsetBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> offsetBufferMemory_;*/
	};

}
