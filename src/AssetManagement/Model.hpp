#pragma once
#include <vulkan/vulkan_core.h>
#include "Procedural.hpp"
#include <string>
#include <memory>

namespace Vulkan::RayTracing
{
    class BottomLevelAccelerationStructure;
    class BottomLevelGeometry;
}

namespace Vulkan
{
    class Buffer;
    class DeviceMemory;
    class CommandBuffers;
	class CommandPool;
}

namespace Assets
{
    class Material;

    class Model final
    {
    public:
        struct VertexData
        {
            const void* data;
            size_t size;
            uint32_t stride;
            uint32_t count;
        };

        struct IndexData
        {
            const void* data;
            size_t size;
            uint32_t count;
        };

        using BLAS = std::shared_ptr<Vulkan::RayTracing::BottomLevelAccelerationStructure>;
        using BufferPtr = std::unique_ptr<Vulkan::Buffer>;
		using DeviceMemoryPtr = std::unique_ptr<Vulkan::DeviceMemory>;

        Model(const std::string& filepath, const VertexData& vertices, const IndexData& indices,
            Vulkan::CommandPool& commandPool, std::shared_ptr<const class Procedural> procedural = nullptr);
        ~Model() = default;

		// Handling gpu resources, so non-copyable
        Model(const Model&) = delete;
        Model& operator=(const Model&) = delete;
        Model(Model&&) noexcept = default;
        Model& operator=(Model&&) noexcept = default;

        void BuildBLAS(Vulkan::CommandPool& commandPool, VkCommandBuffer commandBuffer, Vulkan::Buffer& scratchBuffer, VkDeviceSize scratchOffset);

        const class Procedural* GetProcedural() const { return m_procedural.get(); }
        const std::string& GetFilePath() const { return m_filePath; }
        const BLAS& GetBLAS() const { return m_blas; }

        const Vulkan::Buffer* GetVertexBuffer() const noexcept { return m_vertex.buffer.get(); }
        const Vulkan::Buffer* GetIndexBuffer() const noexcept { return m_index.buffer.get(); }

        uint32_t GetVertexCount() const noexcept { return m_vertexCount; }
        uint32_t GetIndexCount() const noexcept { return m_indexCount; }

        bool IsValid() const noexcept { return m_blas != nullptr; }
        bool HasProcedural() const noexcept { return m_procedural != nullptr; }

    private:

        struct BufferResources
        {
            BufferPtr buffer;
            DeviceMemoryPtr memory;
        };

        BLAS m_blas;
        std::string m_filePath;
        std::shared_ptr<const class Procedural> m_procedural;

        uint32_t m_vertexCount;
        uint32_t m_indexCount;

        BufferResources m_vertex;
        BufferResources m_index;
        BufferResources m_blasData;

        void CreateVertexBuffer(const VertexData& vertices, Vulkan::CommandPool& commandPool);
        void CreateIndexBuffer(const IndexData& indices, Vulkan::CommandPool& commandPool);
        void CreateBLASBuffers(Vulkan::CommandPool& commandPool);
    };
}