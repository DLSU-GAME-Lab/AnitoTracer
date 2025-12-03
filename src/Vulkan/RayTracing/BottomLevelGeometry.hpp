#pragma once

#include "Vulkan/Vulkan.hpp"
#include <vector>

namespace Assets
{
	class Procedural;
	class Scene;
}

namespace Vulkan::RayTracing
{

	class BottomLevelGeometry final
	{
	public:
		const VkAccelerationStructureGeometryKHR& Geometry() const { return m_geometry; }
		const VkAccelerationStructureBuildRangeInfoKHR& BuildOffsetInfo() const { return m_buildOffsetInfo; }

		void AddGeometryTriangles(
			VkDeviceAddress vertexDeviceAddress,
			uint32_t vertexCount,
			VkDeviceAddress indexDeviceAddress,
			uint32_t indexCount);

		void AddGeometryAabb(
			const Assets::Scene& scene,
			uint32_t aabbOffset,
			uint32_t aabbCount,
			bool isOpaque);

	private:

		// The geometry to build, addresses of vertices and indices.
		VkAccelerationStructureGeometryKHR m_geometry;
		
		// the number of elements to build and offsets
		VkAccelerationStructureBuildRangeInfoKHR m_buildOffsetInfo;
	};

}
