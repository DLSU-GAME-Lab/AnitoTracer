#include "Model.hpp"
#include "Vulkan/BufferUtil.hpp"
#include "Vulkan/RayTracing/BottomLevelAccelerationStructure.hpp"
#include "Assets/Vertex.hpp"
#include "RayTracer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <Utilities/Exception.hpp>

using namespace glm;

namespace std
{
    template<> struct hash<Assets::Vertex> final
    {
        size_t operator()(Assets::Vertex const& vertex) const noexcept
        {
            return
                Combine(hash<vec3>()(vertex.Position),
                    Combine(hash<vec3>()(vertex.Normal),
                        Combine(hash<vec2>()(vertex.TexCoord),
                            hash<int>()(vertex.MaterialIndex))));
        }

    private:

        static size_t Combine(size_t hash0, size_t hash1)
        {
            return hash0 ^ (hash1 + 0x9e3779b9 + (hash0 << 6) + (hash0 >> 2));
        }
    };
}

namespace Assets
{
	Assets::Model::Model(const std::string& filepath, const VertexData& vertices, const IndexData& indices, Vulkan::CommandPool& commandPool, std::shared_ptr<const class Procedural> procedural)
        :   m_vertexCount(vertices.count), m_indexCount(indices.count)
	{
		CreateVertexBuffer(vertices, commandPool);
		CreateIndexBuffer(indices, commandPool);
	}

	void Model::BuildBLAS(Vulkan::CommandPool& commandPool, VkCommandBuffer commandBuffer, Vulkan::Buffer& scratchBuffer, VkDeviceSize scratchOffset)
	{
        if (m_blas) return; // Already built
 
        Vulkan::RayTracing::BottomLevelGeometry geometry;
        geometry.AddGeometryTriangles(m_vertex.buffer->GetDeviceAddress(), m_vertexCount, m_index.buffer->GetDeviceAddress(), m_indexCount);

        m_blas = std::make_shared<Vulkan::RayTracing::BottomLevelAccelerationStructure>(
            RayTracer::getInstance()->DeviceProcedures(),
            RayTracer::getInstance()->RayTracingProperties(),
            geometry
        );

		CreateBLASBuffers(commandPool);
        m_blas->Generate(commandBuffer, scratchBuffer, scratchOffset, *m_blasData.buffer, 0);
	}

    void Model::CreateVertexBuffer(const VertexData& vertices, Vulkan::CommandPool& commandPool)
    {
        if (!vertices.data || vertices.size == 0)
        {
            Throw("Invalid vertex data for model: " + m_filePath);
        }

        const auto* vertexPtr = static_cast<const uint8_t*>(vertices.data);
        std::vector<uint8_t> vertexVector(vertexPtr, vertexPtr + vertices.size);

        Vulkan::BufferUtil::CreateDeviceBuffer(
            commandPool,
            (m_filePath + "_vertices").c_str(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            vertexVector,
            m_vertex.buffer,
            m_vertex.memory
        );
    }

    void Model::CreateIndexBuffer(const IndexData& indices, Vulkan::CommandPool& commandPool)
    {
        if (!indices.data || indices.size == 0 || indices.count == 0)
        {
            Throw("Invalid index data for model: " + m_filePath);
        }

        const auto* indexPtr = static_cast<const uint8_t*>(indices.data);
        std::vector<uint8_t> indexVector(indexPtr, indexPtr + indices.size);

        Vulkan::BufferUtil::CreateDeviceBuffer(
            commandPool,
            (m_filePath + "_indices").c_str(),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            indexVector,
            m_index.buffer,
            m_index.memory
        );
    }

    void Model::CreateBLASBuffers(Vulkan::CommandPool& commandPool)
    {
        std::vector<uint8_t> emptyData(m_blas->BuildSizes().accelerationStructureSize);
        Vulkan::BufferUtil::CreateDeviceBuffer(
            commandPool,
            (m_filePath + "_blas").c_str(),
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            emptyData,
            m_blasData.buffer,
            m_blasData.memory);
    }
}
