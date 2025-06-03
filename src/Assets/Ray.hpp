#pragma once

#include "Vertex.hpp"
#include <string>
#include <memory>
#include <string>
#include <vector>

#include "Vulkan/Buffer.hpp"

namespace Assets {
	class Ray final {

	public:
		Ray(Vulkan::CommandPool& commandPool, std::vector<Vertex> vertices);
		~Ray();

		void Update();
		void AddVertex(Vulkan::CommandPool& commandPool, Vertex vertex);

		const Vulkan::Buffer& VertexBuffer() const { return *vertexBuffer_; }
		uint32_t NumberOfVertices() const { return static_cast<uint32_t>(vertices_.size()); }

	private:
		void CreateVertexBuffer(Vulkan::CommandPool& commandPool);
		void ClearVertexBuffer();


		std::vector<Vertex> vertices_;

		std::unique_ptr<Vulkan::Buffer> vertexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> vertexBufferMemory_;
	};
}
