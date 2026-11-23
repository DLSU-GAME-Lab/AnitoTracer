#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include "../Common/DebugLine.hpp"
#include <vector>

class JoltDebugRenderer : public JPH::DebugRenderer {
public:
	JoltDebugRenderer() { Initialize(); }

	void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
	void DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow = ECastShadow::Off) override;
	void DrawText3D(JPH::RVec3Arg inPosition, const JPH::string_view& inString, JPH::ColorArg inColor = JPH::Color::sWhite, float inHeight = 0.5f) override {}

	Batch CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) override;
	Batch CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount) override;
	void DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox& inWorldSpaceBounds, float inLODScaleSq, JPH::ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode = ECullMode::CullBackFace, ECastShadow inCastShadow = ECastShadow::On, EDrawMode inDrawMode = EDrawMode::Solid) override;

	// Call once per frame before PhysicsSystem::DrawBodies, clears last frame's lines.
	void Clear() { mLines.clear(); }

	// Call after PhysicsSystem::DrawBodies to retrieve this frame's collected lines.
	const std::vector<DebugLineVertex>& GetLines() const { return mLines; }

private:
	class BatchImpl : public JPH::RefTargetVirtual {
	public:
		JPH_OVERRIDE_NEW_DELETE
			void AddRef() override { ++mRefCount; }
		void Release() override { if (--mRefCount == 0) delete this; }

		JPH::Array<Triangle> mTriangles;
	private:
		std::atomic<JPH::uint32> mRefCount{ 0 };
	};

	std::vector<DebugLineVertex> mLines;
};