#pragma once

#include "Vulkan/Vulkan.hpp"
#include <memory>
#include <vector>

#include "Engine/LightSystem/Light.h"

struct UserSettings;

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

		RayScene(Vulkan::CommandPool& commandPool, UserSettings& userSettings);
		~RayScene();

		void Update(Vulkan::CommandPool& commandPool);

		const std::vector<Ray*>& Rays() const { return rays_; }

		const Vulkan::Buffer& RayCounterBuffer() const { return *rayCounterBuffer_; }
		const Vulkan::Buffer& RayVertexBuffer() const { return *rayVertexBuffer_; }
		const Vulkan::Buffer& RayInfoBuffer() const { return *rayInfoBuffer_; }

	private:
		uint32_t GetRayCounter(Vulkan::CommandPool& commandPool);

		const uint32_t maxRays_ = 16;

		std::vector<Ray*> rays_;

		std::unique_ptr<Vulkan::Buffer> rayCounterBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> rayCounterBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> rayVertexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> rayVertexBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> rayInfoBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> rayInfoBufferMemory_;

		bool hasRenderedRays_ = false;
	};

}
