#pragma once
#include "glm/glm.hpp"
#include "Assets/Vertex.hpp"

class AABB
{
public:
	struct Bounds
	{
		glm::vec3 min;
		glm::vec3 max;
	};

	AABB(const std::vector<Assets::Vertex>& vertices, const glm::mat4& worldMat);
	void Update(const glm::mat4& worldMat);

	Bounds GetUnionBounds() const;

private:
	Bounds localBounds;
	Bounds previousWorldBounds;
	Bounds currentWorldBounds;

	Bounds ComputeBoundingBox(const std::vector<Assets::Vertex>& vertices);
	Bounds TransformBounds(const Bounds& bounds, const glm::mat4& matrix);
};

