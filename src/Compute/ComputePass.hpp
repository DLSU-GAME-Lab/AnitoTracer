#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include "Vulkan/Buffer.hpp"
#include "Vulkan/DeviceMemory.hpp"
#include "Vulkan/CommandBuffers.hpp"
#include "ComputePipeline.hpp"

namespace Vulkan
{
    class Device;
    class SwapChain;
    class CommandPool;
    class Fence;
}

class ComputePipeline;

struct PushConstantsWorkLoader
{
    uint32_t width;
    uint32_t height;
    float probability;
    uint32_t pixelCount;
};

struct PixelWorkItem
{
    uint32_t x;
    uint32_t y;
};

class ComputePass
{
public:
	using Buffer = Vulkan::Buffer;
	using DeviceMemory = Vulkan::DeviceMemory;
	using CommandBuffers = Vulkan::CommandBuffers;

    ComputePass(const Vulkan::Device* device,
        const Vulkan::SwapChain& swapChain,
        Vulkan::CommandPool& commandPool);

    ~ComputePass();
    void ProcessComputePass(uint32_t frameIndex);

	VkBuffer getWorkQueueBuffer() const { return workQueueBuffer.buffer->Handle(); }
	VkBuffer getWorkQueueCountBuffer() const { return workQueueCountBuffer.buffer->Handle(); }

private:
    void RecordComputeCommands(VkCommandBuffer cmd, uint32_t frameIndex);
    void RecordLoadWorkPass(VkCommandBuffer cmd, uint32_t frameIndex);

private:
    struct BufferBundle
    {
        std::unique_ptr<Vulkan::Buffer> buffer;
        std::unique_ptr<Vulkan::DeviceMemory> memory;
    };

    const Vulkan::Device* device_ = nullptr;

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t numberOfFrames = 0;

    BufferBundle workQueueBuffer;
    BufferBundle workQueueCountBuffer;
    BufferBundle scratchBuffer;

    std::unique_ptr<Vulkan::CommandBuffers> commandBuffers;
    std::unique_ptr<ComputePipeline> computePipeline;

    std::vector<Vulkan::Fence> inFlightFences;
};
