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

class GameObject;

namespace Assets
{
	class Model;
	class Texture;
	class TextureImage;

	class Scene final
	{
	public:

		Scene(const Scene&) = delete;
		Scene(Scene&&) = delete;
		Scene& operator = (const Scene&) = delete;
		Scene& operator = (Scene&&) = delete;

		Scene(Vulkan::CommandPool& commandPool, std::vector<GameObject*>&& gameObjects,
			std::vector<Texture>&& textures, std::vector<LightProperties>&& lights);
		~Scene();

		const std::vector<GameObject*>& GameObjects() const { return gameObjects_; }
		bool HasProcedurals() const { return static_cast<bool>(proceduralBuffer_); }

		const Vulkan::Buffer& VertexBuffer() const { return *vertexBuffer_; }
		const Vulkan::Buffer& IndexBuffer() const { return *indexBuffer_; }
		const Vulkan::Buffer& MaterialBuffer() const { return *materialBuffer_; }
		const Vulkan::Buffer& LightBuffer() const { return *lightsBuffer_; }
		const Vulkan::Buffer& OffsetsBuffer() const { return *offsetBuffer_; }
		const Vulkan::Buffer& AabbBuffer() const { return *aabbBuffer_; }
		const Vulkan::Buffer& ProceduralBuffer() const { return *proceduralBuffer_; }
		const std::vector<VkImageView> TextureImageViews() const { return textureImageViewHandles_; }
		const std::vector<VkSampler> TextureSamplers() const { return textureSamplerHandles_; }

		void SetSkybox(VkImageView imageView, VkSampler sampler);

		VkImageView SkyboxImageView() const { return skyboxImageView_; }
		VkSampler SkyboxSampler() const { return skyboxSampler_; }

	private:

		const std::vector<GameObject*> gameObjects_;
		const std::vector<Texture> textures_;
		const std::vector<LightProperties> lights_;

		std::unique_ptr<Vulkan::Buffer> vertexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> vertexBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> indexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> indexBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> materialBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> materialBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> offsetBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> offsetBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> aabbBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> aabbBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> proceduralBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> proceduralBufferMemory_;

		std::unique_ptr<Vulkan::Buffer> lightsBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> lightsBufferMemory_;

		std::vector<std::unique_ptr<TextureImage>> textureImages_;
		std::vector<VkImageView> textureImageViewHandles_;
		std::vector<VkSampler> textureSamplerHandles_;

		std::unique_ptr<TextureImage> skyboxTexture_;
		VkImageView skyboxImageView_ = VK_NULL_HANDLE;
		VkSampler skyboxSampler_ = VK_NULL_HANDLE;
	};

}
