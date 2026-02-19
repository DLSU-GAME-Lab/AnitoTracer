#include "ComputePass.hpp"
#include "ComputePipelineLayout.hpp"
#include "ComputePipeline.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/MemoryBarrierUtil.hpp"
#include "Vulkan/BufferUtil.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Fence.hpp"

#include <vulkan/vulkan.h>
#include <limits>
#include <From-GDGRAP2/Debug.h>
#include <Utilities/FileUtils.h>

namespace
{
    constexpr uint32_t kTileSize = 16;

    inline uint32_t DivUp(uint32_t a, uint32_t b)
    {
        return (a + b - 1u) / b;
    }
}

ComputePass::ComputePass(const Vulkan::Device* device,
    const Vulkan::SwapChain& swapChain,
    Vulkan::CommandPool& commandPool)
    : device_(device)
{
    width = swapChain.Extent().width;
    height = swapChain.Extent().height;

    numberOfFrames = static_cast<uint32_t>(swapChain.ImageViews().size());

    const uint32_t pixelCount = width * height;
    const VkDeviceSize workItemsBytes = VkDeviceSize(pixelCount) * sizeof(PixelWorkItem);

    // Storage buffers used by compute
    workQueueBuffer.buffer = std::make_unique<Buffer>(swapChain.Device(),
        workItemsBytes,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    workQueueBuffer.memory = std::make_unique<DeviceMemory>(workQueueBuffer.buffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

    workQueueCountBuffer.buffer = std::make_unique<Buffer>(swapChain.Device(),
        sizeof(uint32_t),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    workQueueCountBuffer.memory = std::make_unique<DeviceMemory>(workQueueCountBuffer.buffer->AllocateMemory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

    // Host-visible readback buffer
    scratchBuffer.buffer = std::make_unique<Buffer>(swapChain.Device(),
        workItemsBytes,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    scratchBuffer.memory = std::make_unique<DeviceMemory>(scratchBuffer.buffer->AllocateMemory(
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

    commandBuffers = std::make_unique<Vulkan::CommandBuffers>(commandPool, numberOfFrames);

    // init pipelines
	computePipeline = std::make_unique<ComputePipeline>(*device_, numberOfFrames);

	// 0 = work loader pass
	computePipeline->CreatePipeline(FileUtils::getAssetsFolderPath().generic_string() + "/shaders/WorkLoader.comp.spv");

    inFlightFences.reserve(numberOfFrames);
    for (uint32_t i = 0; i < numberOfFrames; ++i)
    {
        inFlightFences.emplace_back(*device_, true); 
    }
}

ComputePass::~ComputePass()
{
    computePipeline.reset();
    commandBuffers.reset();
    scratchBuffer.memory.reset();
    scratchBuffer.buffer.reset();
    workQueueCountBuffer.memory.reset();
    workQueueCountBuffer.buffer.reset();
    workQueueBuffer.memory.reset();
    workQueueBuffer.buffer.reset();
}

void ComputePass::ProcessComputePass(uint32_t frameIndex)
{
    constexpr uint64_t kNoTimeout = std::numeric_limits<uint64_t>::max();

    auto& fence = inFlightFences[frameIndex];

    fence.Wait(kNoTimeout);
    fence.Reset();

    VkCommandBuffer cmd = commandBuffers->Begin(frameIndex);

    RecordComputeCommands(cmd, frameIndex);

    commandBuffers->End(frameIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(device_->ComputeQueue(), 1, &submitInfo, fence.Handle());

    // Keeps your current “CPU waits for completion” behavior.
    fence.Wait(kNoTimeout);

    /* ===== DEBUG: Read back generated PixelWorkItems ===== */
    //{
    //    const uint32_t pixelCount = width * height;
    //    const VkDeviceSize bytes = VkDeviceSize(pixelCount) * sizeof(PixelWorkItem);

    //    std::vector<PixelWorkItem> result(pixelCount);

    //    void* mapped = scratchBuffer.memory->Map(0, bytes);
    //    std::memcpy(result.data(), mapped, static_cast<size_t>(bytes));
    //    scratchBuffer.memory->Unmap();

    //    for (uint32_t i = 0; i < pixelCount; ++i)
    //    {
    //        Debug::Log(std::to_string(result[i].x) + " " + std::to_string(result[i].y));
    //    }
    //}
    /* ===== END DEBUG ===== */
}

void ComputePass::RecordComputeCommands(VkCommandBuffer cmd, uint32_t frameIndex)
{
    // Future-friendly: add more passes here.
    // RecordSomeOtherComputePass(cmd, frameIndex);
    RecordLoadWorkPass(cmd, frameIndex);
}

void ComputePass::RecordLoadWorkPass(VkCommandBuffer cmd, uint32_t frameIndex)
{
    const uint32_t groupCountX = DivUp(width, kTileSize);
    const uint32_t groupCountY = DivUp(height, kTileSize);

    const uint32_t pixelCount = width * height;
    const VkDeviceSize bytes = VkDeviceSize(pixelCount) * sizeof(PixelWorkItem);

    // Clear buffers via transfer
    vkCmdFillBuffer(cmd, workQueueBuffer.buffer->Handle(), 0, bytes, 0u);
    vkCmdFillBuffer(cmd, workQueueCountBuffer.buffer->Handle(), 0, sizeof(uint32_t), 0u);

    const std::vector<Vulkan::BufferMemoryBarrier::BufferRange> queueRanges = {
        { workQueueBuffer.buffer->Handle(),      0, VK_WHOLE_SIZE },
        { workQueueCountBuffer.buffer->Handle(), 0, VK_WHOLE_SIZE },
    };

    // Transfer -> Compute: make fills visible to shader
    Vulkan::BufferMemoryBarrier::Insert(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        queueRanges
    );

	const std::vector<Vulkan::Buffer*> buffers = { workQueueBuffer.buffer.get(), workQueueCountBuffer.buffer.get() };

	computePipeline->UpdateDescriptorSet(buffers, frameIndex);
    VkDescriptorSet ds = computePipeline->DescriptorSet(frameIndex);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline->GetPipeline(0));
    vkCmdBindDescriptorSets(
        cmd,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        computePipeline->PipelineLayout().Handle(),
        0, 1, &ds,
        0, nullptr
    );

    // Push constants
    const PushConstantsWorkLoader pc{ width, height, 1.0f, frameIndex };

    vkCmdPushConstants(
        cmd,
        computePipeline->PipelineLayout().Handle(),
        VK_SHADER_STAGE_COMPUTE_BIT,
        0, sizeof(PushConstantsWorkLoader),
        &pc
    );

    vkCmdDispatch(cmd, groupCountX, groupCountY, 1);

    // Compute -> Ray Tracing (KHR): make compute writes visible to rgen (and other RT shaders)
    Vulkan::BufferMemoryBarrier::Insert(
        cmd,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        queueRanges
    );

	{   // Readback to CPU for debugging/analysis
        // Compute -> Transfer: make shader writes visible for copy
        //Vulkan::BufferMemoryBarrier::Insert(
        //    cmd,
        //    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        //    VK_PIPELINE_STAGE_TRANSFER_BIT,
        //    VK_ACCESS_SHADER_WRITE_BIT,
        //    VK_ACCESS_TRANSFER_READ_BIT,
        //    queueRanges
        //);

        //// Copy work queue to host-visible scratch
        //VkBufferCopy copy{};
        //copy.srcOffset = 0;
        //copy.dstOffset = 0;
        //copy.size = bytes;

        //vkCmdCopyBuffer(cmd,
        //    workQueueBuffer.buffer->Handle(),
        //    scratchBuffer.buffer->Handle(),
        //    1,
        //    &copy);

        //// Transfer -> Host: make copy visible for CPU readback
        //Vulkan::BufferMemoryBarrier::Insert(
        //    cmd,
        //    VK_PIPELINE_STAGE_TRANSFER_BIT,
        //    VK_PIPELINE_STAGE_HOST_BIT,
        //    VK_ACCESS_TRANSFER_WRITE_BIT,
        //    VK_ACCESS_HOST_READ_BIT,
        //    scratchBuffer.buffer->Handle(),
        //    0,
        //    bytes
        //);
    }
}
