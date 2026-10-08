#include "JoltDebugRenderer.hpp"

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) {
	glm::vec4 color(inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f, inColor.a / 255.0f);

	mLines.push_back({ glm::vec3(inFrom.GetX(), inFrom.GetY(), inFrom.GetZ()), color });
	mLines.push_back({ glm::vec3(inTo.GetX(), inTo.GetY(), inTo.GetZ()), color });
}

void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow) {
	// Render triangles as their three edges
	DrawLine(inV1, inV2, inColor);
	DrawLine(inV2, inV3, inColor);
	DrawLine(inV3, inV1, inColor);
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const Triangle* inTriangles, int inTriangleCount) {
	auto* batch = new BatchImpl;
	batch->mTriangles.assign(inTriangles, inTriangles + inTriangleCount);
	return batch;
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount) {
	auto* batch = new BatchImpl;
	batch->mTriangles.resize(inIndexCount / 3);
	for (size_t t = 0; t < batch->mTriangles.size(); ++t)
		for (int v = 0; v < 3; ++v)
			batch->mTriangles[t].mV[v] = inVertices[inIndices[t * 3 + v]];
	return batch;
}

void JoltDebugRenderer::DrawGeometry(JPH::RMat44Arg inModelMatrix, const JPH::AABox&, float, JPH::ColorArg inModelColor,
	const GeometryRef& inGeometry, ECullMode, ECastShadow, EDrawMode) {
	if (inGeometry->mLODs.empty()) return;

	const auto* batch = static_cast<const BatchImpl*>(inGeometry->mLODs.back().mTriangleBatch.GetPtr());
	for (const Triangle& tri : batch->mTriangles) {
		JPH::RVec3 a = inModelMatrix * JPH::Vec3(tri.mV[0].mPosition);
		JPH::RVec3 b = inModelMatrix * JPH::Vec3(tri.mV[1].mPosition);
		JPH::RVec3 c = inModelMatrix * JPH::Vec3(tri.mV[2].mPosition);
		DrawTriangle(a, b, c, inModelColor, ECastShadow::Off);
	}
}