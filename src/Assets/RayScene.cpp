#include "RayScene.hpp"

#include <iostream>
#include <random>

#include "Ray.hpp"
#include "RayVertex.hpp"
#include "Scene.hpp"
#include "Model.hpp"
#include "From-GDGRAP2/Debug.h"
#include "Vulkan/BufferUtil.hpp"
#include "Utilities/Exception.hpp"
#include "Vulkan/SingleTimeCommands.hpp"

using namespace glm;
namespace Assets {
	RayScene::RayScene(Vulkan::CommandPool& commandPool)
	{
	constexpr auto flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	rayVertexBuffer_.reset(new Vulkan::Buffer(commandPool.Device(), sizeof(RayVertex) * 64, flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
	rayVertexBufferMemory_.reset(new Vulkan::DeviceMemory(rayVertexBuffer_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));

	//rayCounterBuffer_.reset(new Vulkan::Buffer(commandPool.Device(), sizeof(uint32_t), flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
	//rayCounterBufferMemory_.reset(new Vulkan::DeviceMemory(rayDebugBuffer_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
	
	std::vector<uint32_t> rayCounter = { 0 };

	Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Ray Counter", flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, rayCounter, rayCounterBuffer_, rayCounterBufferMemory_);

	rayIndexBuffer_.reset(new Vulkan::Buffer(commandPool.Device(), sizeof(uint32_t) * 64, flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
	rayIndexBufferMemory_.reset(new Vulkan::DeviceMemory(rayIndexBuffer_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));

	rayInfoBuffer_.reset(new Vulkan::Buffer(commandPool.Device(), sizeof(RayInfo) * 16, flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
	rayInfoBufferMemory_.reset(new Vulkan::DeviceMemory(rayInfoBuffer_->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
	//Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Ray Info", flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, rayInfo, rayInfoBuffer_, rayInfoBufferMemory_);
}

RayScene::~RayScene()
{
	rays_.clear();
	//vertexBuffer_.reset();
	//vertexBufferMemory_.reset(); // release memory after bound buffer has been destroyed
}

void RayScene::Update(Vulkan::CommandPool& commandPool)
{
	uint32_t numRays = GetRayCounter(commandPool);
	if (numRays == 0 || numRays > maxRays_) return;

	const auto contentSize = sizeof(RayVertex) * maxRays_;

	auto stagingBuffer = std::make_unique<Vulkan::Buffer>(commandPool.Device(), contentSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	auto stagingBufferMemory = stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	stagingBuffer->CopyFrom(commandPool, *rayVertexBuffer_, contentSize);

	RayVertex rayVertices[16];
	const auto data = stagingBufferMemory.Map(0, contentSize);
	std::memcpy(&rayVertices, data, contentSize);
	stagingBufferMemory.Unmap();

	stagingBuffer.reset();

	const auto indexContentSize = sizeof(uint32_t) * 64;

	auto indexStagingBuffer = std::make_unique<Vulkan::Buffer>(commandPool.Device(), indexContentSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	auto indexStagingBufferMemory = indexStagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	indexStagingBuffer->CopyFrom(commandPool, *rayIndexBuffer_, indexContentSize);

	uint32_t rayIndices[64];
	const auto indexData = indexStagingBufferMemory.Map(0, indexContentSize);
	std::memcpy(&rayIndices, indexData, contentSize);
	indexStagingBufferMemory.Unmap();

	indexStagingBuffer.reset();

	if (numRays == maxRays_ && !alreadyPut_)
	{
		alreadyPut_ = true;
		std::vector<Vertex> vertices;
		for (int i = 0; i < 4; i++)
		{
			//Debug::Log("Index " + std::to_string(rayIndices[i]) + "\n");
			Debug::Log("Vertex " + std::to_string(i) + " (" + std::to_string(rayVertices[i].Position.x) + ", " + std::to_string(rayVertices[i].Position.y) + ", " + std::to_string(rayVertices[i].Position.z) + ")\n");
			vertices.push_back(Vertex{ rayVertices[i].Position, vec3(0, 0, 0), vec2(0, 0), -1});
		}
		rays_.push_back(new Ray(commandPool, vertices));
	}
}

uint32_t RayScene::GetRayCounter(Vulkan::CommandPool& commandPool)
{
	const auto contentSize = sizeof(uint32_t);

	auto stagingBuffer = std::make_unique<Vulkan::Buffer>(commandPool.Device(), contentSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	auto stagingBufferMemory = stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	stagingBuffer->CopyFrom(commandPool, *rayCounterBuffer_, contentSize);

	uint32_t numRays;
	const auto data = stagingBufferMemory.Map(0, contentSize);
	std::memcpy(&numRays, data, contentSize);
	stagingBufferMemory.Unmap();

	stagingBuffer.reset();

	return numRays;
}
}
