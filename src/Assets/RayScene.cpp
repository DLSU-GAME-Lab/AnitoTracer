#include "RayScene.hpp"

#include "Ray.hpp"
#include "RayVertex.hpp"
#include "RayInfo.hpp"
#include "UserSettings.hpp"

#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/EventBroadcaster.h"
#include "From-GDGRAP2/EventNames.h"
#include "Vulkan/BufferUtil.hpp"
#include "Vulkan/SingleTimeCommands.hpp"
#include <iostream>

using namespace glm;
namespace Assets {
	RayScene::RayScene(Vulkan::CommandPool& commandPool, UserSettings& userSettings) :
		maxRays_(userSettings.MaxRays)
	{

		constexpr auto flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

		// Create ray vertex buffer to be placed into the ray tracing pipeline as storage for calculated ray positions.
		rayVertexBuffer_.reset(new Vulkan::Buffer(commandPool.Device(), sizeof(RayVertex) * 512, flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
		rayVertexBufferMemory_.reset(new Vulkan::DeviceMemory(rayVertexBuffer_->AllocateMemory(VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));

		// Ray counter buffer to count how many rays have been calculated in the GPU
		std::vector<uint32_t> rayCounter = { 0 };
		Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Ray Counter", flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, rayCounter, rayCounterBuffer_, rayCounterBufferMemory_);

		// Storage buffer for ray information
		rayInfoBuffer_.reset(new Vulkan::Buffer(commandPool.Device(), sizeof(RayInfo) * maxRays_, flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
		rayInfoBufferMemory_.reset(new Vulkan::DeviceMemory(rayInfoBuffer_->AllocateMemory(VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)));
	}

	RayScene::~RayScene()
	{
		rays_.clear();
	
		rayCounterBuffer_.reset();
		rayCounterBufferMemory_.reset();
	
		rayVertexBuffer_.reset();
		rayVertexBufferMemory_.reset();
	
		rayInfoBuffer_.reset();
		rayInfoBufferMemory_.reset();
	}

	void RayScene::Update(Vulkan::CommandPool& commandPool)
	{
		uint32_t numRays = GetRayCounter(commandPool);

		if (numRays == maxRays_ && rays_.size() != maxRays_ && !hasRenderedRays_)
		{
			// Broadcast RAYS_START_RENDER event
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::RAYS_START_RENDER);
			const auto contentSize = sizeof(RayVertex) * 512;
	
			auto stagingBuffer = std::make_unique<Vulkan::Buffer>(commandPool.Device(), contentSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
			auto stagingBufferMemory = stagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
			stagingBuffer->CopyFrom(commandPool, *rayVertexBuffer_, contentSize);
	
			RayVertex rayVertices[512];
			const auto data = stagingBufferMemory.Map(0, contentSize);
			std::memcpy(&rayVertices, data, contentSize);
			stagingBufferMemory.Unmap();
	
			stagingBuffer.reset();
	
			const auto infoContentSize = sizeof(RayInfo) * maxRays_;
	
			auto infoStagingBuffer = std::make_unique<Vulkan::Buffer>(commandPool.Device(), infoContentSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
			auto infoStagingBufferMemory = infoStagingBuffer->AllocateMemory(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	
			infoStagingBuffer->CopyFrom(commandPool, *rayInfoBuffer_, infoContentSize);
	
			RayInfo* rayInfos = new RayInfo[maxRays_]();
			const auto infoData = infoStagingBufferMemory.Map(0, infoContentSize);
			std::memcpy(rayInfos, infoData, infoContentSize);
			infoStagingBufferMemory.Unmap();
	
			infoStagingBuffer.reset();

			for (uint32_t i = 0; i < maxRays_; i++)
			{
				std::vector<RayVertex> vertices;
				for (uint32_t j = 0; j < rayInfos[i].RayCount; j++)
				{
					const uint32_t offset = rayInfos[i].RayOffset;
	
					vertices.push_back(RayVertex{ rayVertices[offset * i + j].Position, 0, rayVertices[offset * i + j].Color });
				}
				rays_.push_back(new Ray(commandPool, vertices));
			}
			hasRenderedRays_ = true;

			std::cout << "Rays rendered desu " << std::endl;

			delete[] rayInfos;

			// Broadcast RAYS_END_RENDER event
			EventBroadcaster::getInstance()->broadcastEvent(EventNames::RAYS_END_RENDER);
		}
	}

	uint32_t RayScene::GetRayCounter(Vulkan::CommandPool& commandPool)
	{
		// Wait for device to complete all pending operations before reading counter
		vkDeviceWaitIdle(commandPool.Device().Handle());

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
