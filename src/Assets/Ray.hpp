#pragma once

#include <string>
#include <memory>
#include <string>
#include <vector>

#include "RayVertex.hpp"
#include "Vulkan/Buffer.hpp"

namespace Assets {
	class Ray final {

	public:
		Ray(Vulkan::CommandPool& commandPool, std::vector<RayVertex> vertices);
		~Ray();

		void Update();
		void Reset();
		void AddVertex(Vulkan::CommandPool& commandPool, RayVertex vertex);

		const Vulkan::Buffer& VertexBuffer() const { return *vertexBuffer_; }
		uint32_t NumberOfVertices() const { return static_cast<uint32_t>(vertices_.size()); }

	private:
		void CreateVertexBuffer(Vulkan::CommandPool& commandPool);
		void ClearVertexBuffer();


		std::vector<RayVertex> vertices_;

		std::unique_ptr<Vulkan::Buffer> vertexBuffer_;
		std::unique_ptr<Vulkan::DeviceMemory> vertexBufferMemory_;
	};
}
