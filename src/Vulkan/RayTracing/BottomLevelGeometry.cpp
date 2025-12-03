#include "BottomLevelGeometry.hpp"
#include "DeviceProcedures.hpp"
#include "Assets/Scene.hpp"
#include "Assets/Vertex.hpp"
#include "Vulkan/Buffer.hpp"

namespace Vulkan::RayTracing {

void BottomLevelGeometry::AddGeometryTriangles(
	VkDeviceAddress vertexDeviceAddress, const uint32_t vertexCount,
	VkDeviceAddress indexDeviceAddress, const uint32_t indexCount)
{
	VkAccelerationStructureGeometryKHR geometry = {};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.pNext = nullptr;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	geometry.geometry.triangles.pNext = nullptr;
	geometry.geometry.triangles.vertexData.deviceAddress = vertexDeviceAddress;
	geometry.geometry.triangles.vertexStride = sizeof(Assets::Vertex);
	geometry.geometry.triangles.maxVertex = vertexCount;
	geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	geometry.geometry.triangles.indexData.deviceAddress = indexDeviceAddress;
	geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.transformData = {};
	geometry.flags = 0;

	VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo = {};
	buildOffsetInfo.firstVertex = 0;
	buildOffsetInfo.primitiveOffset = 0;
	buildOffsetInfo.primitiveCount = indexCount / 3;
	buildOffsetInfo.transformOffset = 0;

	m_geometry = geometry;
	m_buildOffsetInfo = buildOffsetInfo;
}

void BottomLevelGeometry::AddGeometryAabb(
	const Assets::Scene& scene,
	const uint32_t aabbOffset,
	const uint32_t aabbCount,
	const bool isOpaque)
{
	//VkAccelerationStructureGeometryKHR geometry = {};
	//geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	//geometry.pNext = nullptr;
	//geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
	//geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
	//geometry.geometry.aabbs.pNext = nullptr;
	//geometry.geometry.aabbs.data.deviceAddress = scene.AabbBuffer().GetDeviceAddress();
	//geometry.geometry.aabbs.stride = sizeof(VkAabbPositionsKHR);
	//geometry.flags = 0;

	//VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo = {};
	//buildOffsetInfo.firstVertex = 0;
	//buildOffsetInfo.primitiveOffset = aabbOffset;
	//buildOffsetInfo.primitiveCount = aabbCount;
	//buildOffsetInfo.transformOffset = 0;

	//geometry_.emplace_back(geometry);
	//buildOffsetInfo_.emplace_back(buildOffsetInfo);
}

}
