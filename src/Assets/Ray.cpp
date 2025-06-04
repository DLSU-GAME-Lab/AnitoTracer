#include "Ray.hpp"

#include "Vulkan/BufferUtil.hpp"

namespace Assets
{
	Ray::Ray(Vulkan::CommandPool& commandPool, std::vector<Vertex> vertices)
	{
		vertices_.insert(vertices_.end(), vertices.begin(), vertices.end());

		CreateVertexBuffer(commandPool);
	}

	Ray::~Ray()
	{
		vertexBuffer_.reset();
		vertexBufferMemory_.reset();
	}

	void Ray::Update()
	{

	}

	void Ray::Reset() {
		vertices_.clear();
	}

	void Ray::AddVertex(Vulkan::CommandPool& commandPool, Vertex vertex)
	{
		vertices_.push_back(vertex);

		Vulkan::BufferUtil::CopyFromStagingBuffer(commandPool, *vertexBuffer_, vertices_);
	}

	void Ray::CreateVertexBuffer(Vulkan::CommandPool& commandPool)
	{
		constexpr auto flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		Vulkan::BufferUtil::CreateDeviceBuffer(commandPool, "Vertices", VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | flags, vertices_, vertexBuffer_, vertexBufferMemory_);
	}

	void Ray::ClearVertexBuffer()
	{
		vertexBuffer_.reset();
		vertexBufferMemory_.reset();
	}
}
