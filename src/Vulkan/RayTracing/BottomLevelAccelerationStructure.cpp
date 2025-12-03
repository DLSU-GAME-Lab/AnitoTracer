#include "BottomLevelAccelerationStructure.hpp"
#include "DeviceProcedures.hpp"
#include "Assets/Scene.hpp"
#include "Assets/Vertex.hpp"
#include "Utilities/Exception.hpp"
#include "Vulkan/Buffer.hpp"

namespace Vulkan::RayTracing {

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(
	const class DeviceProcedures& deviceProcedures,
	const class RayTracingProperties& rayTracingProperties,
	const BottomLevelGeometry& geometries) :
	AccelerationStructure(deviceProcedures, rayTracingProperties),
	m_geometry(geometries)
{
	buildGeometryInfo_.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	buildGeometryInfo_.flags = flags_;
	buildGeometryInfo_.geometryCount = 1;
	buildGeometryInfo_.pGeometries = &m_geometry.Geometry();
	buildGeometryInfo_.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	buildGeometryInfo_.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	buildGeometryInfo_.srcAccelerationStructure = nullptr;
	
	buildSizesInfo_ = GetBuildSizes(&m_geometry.BuildOffsetInfo().primitiveCount);
}

BottomLevelAccelerationStructure::BottomLevelAccelerationStructure(BottomLevelAccelerationStructure&& other) noexcept :
	AccelerationStructure(std::move(other)),
	m_geometry(std::move(other.m_geometry))
{
}

BottomLevelAccelerationStructure::~BottomLevelAccelerationStructure()
{
}

void BottomLevelAccelerationStructure::Generate(
	VkCommandBuffer commandBuffer,
	Buffer& scratchBuffer,
	const VkDeviceSize scratchOffset,
	Buffer& resultBuffer,
	const VkDeviceSize resultOffset)
{
	// Create the acceleration structure.
	CreateAccelerationStructure(resultBuffer, resultOffset);

	// Build the actual bottom-level acceleration structure
	const VkAccelerationStructureBuildRangeInfoKHR* pBuildOffsetInfo = &m_geometry.BuildOffsetInfo();

	buildGeometryInfo_.dstAccelerationStructure = Handle();
	buildGeometryInfo_.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress() + scratchOffset;

	deviceProcedures_.vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildGeometryInfo_, &pBuildOffsetInfo);
}

}
