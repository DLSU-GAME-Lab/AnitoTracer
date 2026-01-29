#include "Scene.hpp"

#include <iostream>

#include "From-GDGRAP2/GameObject.h"
#include "SphereProc.hpp"
#include "Texture.hpp"
#include "TextureImage.hpp"
#include "Engine/LightSystem/Light.h"
#include "Vulkan/BufferUtil.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Utilities/Exception.hpp"
#include "Vulkan/SingleTimeCommands.hpp"


namespace Assets {

	Scene::Scene(Vulkan::CommandPool& commandPool, std::vector<GameObject*>&& gameObjects, std::vector<Texture>&& textures, std::vector<LightProperties>&& lights) :
		gameObjects_(std::move(gameObjects)),
		textures_(std::move(textures)),
		lights_(std::move(lights))
	{
		// Concatenate all the models
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<Material> materials;
		std::vector<glm::vec4> procedurals;
		std::vector<VkAabbPositionsKHR> aabbs;
		std::vector<glm::uvec2> offsets;

		for (const auto& obj : gameObjects_)
		{
			if (!obj->getModel()) continue;

			// Remember the index, vertex offsets.
			const auto indexOffset = static_cast<uint32_t>(indices.size());
			const auto vertexOffset = static_cast<uint32_t>(vertices.size());
			const auto materialOffset = static_cast<uint32_t>(materials.size());

			offsets.push_back({ indexOffset, vertexOffset });

			// Copy model data one after the other.
			vertices.insert(vertices.end(), obj->getModel()->Vertices().begin(), obj->getModel()->Vertices().end());
			indices.insert(indices.end(), obj->getModel()->Indices().begin(), obj->getModel()->Indices().end());
			materials.insert(materials.end(), obj->getModel()->Materials().begin(), obj->getModel()->Materials().end());

			// Adjust the material id.
			for (size_t i = vertexOffset; i != vertices.size(); ++i)
			{
				vertices[i].MaterialIndex += materialOffset;
			}

			// Add optional procedurals.
			const auto* const sphere = dynamic_cast<const SphereProc*>(obj->getModel()->Procedural());
			if (sphere != nullptr)
			{
				const auto aabb = sphere->BoundingBox();
				aabbs.push_back({ aabb.first.x, aabb.first.y, aabb.first.z, aabb.second.x, aabb.second.y, aabb.second.z });
				procedurals.emplace_back(sphere->Center, sphere->Radius);
			}
			else
			{
				aabbs.emplace_back();
				procedurals.emplace_back();
			}
		}

		std::vector<LightProperties> lightProps;
		for (const auto& l : lights_)
		{
			lightProps.emplace_back(l);
		}

		constexpr auto flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Vertices", VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | flags, vertices, vertexBuffer_, vertexBufferMemory_);
		Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Indices", VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | flags, indices, indexBuffer_, indexBufferMemory_);
		Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Materials", flags, materials, materialBuffer_, materialBufferMemory_);
		Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Lights", flags, lightProps, lightsBuffer_, lightsBufferMemory_);
		Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Offsets", flags, offsets, offsetBuffer_, offsetBufferMemory_);

		Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "AABBs", VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | flags, aabbs, aabbBuffer_, aabbBufferMemory_);
		Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Procedurals", flags, procedurals, proceduralBuffer_, proceduralBufferMemory_);

		// Upload all textures
		textureImages_.reserve(textures_.size());
		textureImageViewHandles_.resize(textures_.size());
		textureSamplerHandles_.resize(textures_.size());

		for (size_t i = 0; i != textures_.size(); ++i)
		{
			textureImages_.emplace_back(new TextureImage(commandPool, textures_[i]));
			textureImageViewHandles_[i] = textureImages_[i]->ImageView().Handle();
			textureSamplerHandles_[i] = textureImages_[i]->Sampler().Handle();
		}
	}

	void Scene::SetSkybox(VkImageView imageView, VkSampler sampler)
	{
		skyboxImageView_ = imageView;
		skyboxSampler_ = sampler;
	}

	Scene::~Scene()
	{
		textureSamplerHandles_.clear();
		textureImageViewHandles_.clear();
		textureImages_.clear();
		proceduralBuffer_.reset();
		proceduralBufferMemory_.reset(); // release memory after bound buffer has been destroyed
		aabbBuffer_.reset();
		aabbBufferMemory_.reset(); // release memory after bound buffer has been destroyed
		offsetBuffer_.reset();
		offsetBufferMemory_.reset(); // release memory after bound buffer has been destroyed
		materialBuffer_.reset();
		materialBufferMemory_.reset(); // release memory after bound buffer has been destroyed
		indexBuffer_.reset();
		indexBufferMemory_.reset(); // release memory after bound buffer has been destroyed
		vertexBuffer_.reset();
		vertexBufferMemory_.reset(); // release memory after bound buffer has been destroyed
		lightsBuffer_.reset();
		lightsBufferMemory_.reset();
	}

}