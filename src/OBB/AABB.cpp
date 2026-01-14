#include "AABB.hpp"

AABB::AABB(const std::vector<Assets::Vertex>& vertices, const glm::mat4& worldMat)
{
    this->localBounds = ComputeBoundingBox(vertices);
	this->currentWorldBounds = TransformBounds(this->localBounds, worldMat);
	this->previousWorldBounds = this->currentWorldBounds;
}

AABB::Bounds AABB::GetUnionBounds() const
{
    Bounds result;
    result.min = glm::min(this->previousWorldBounds.min, this->currentWorldBounds.min);
    result.max = glm::max(this->previousWorldBounds.max, this->currentWorldBounds.max);
	return result;
}

AABB::Bounds AABB::ComputeBoundingBox(const std::vector<Assets::Vertex>& vertices)
{
    glm::vec3 vmin = vertices[0].Position;
    glm::vec3 vmax = vmin;

    for (size_t i = 1; i < vertices.size(); i++)
    {
        const glm::vec3& current = vertices[i].Position;
        vmin = glm::min(vmin, current);
        vmax = glm::max(vmax, current);
    }

    Bounds result;
    result.min = vmin;
    result.max = vmax;
    return result;
}

AABB::Bounds AABB::TransformBounds(const Bounds& bounds, const glm::mat4& matrix)
{
    glm::vec3 corners[8];

    corners[0] = glm::vec3(bounds.min.x, bounds.min.y, bounds.min.z);
    corners[1] = glm::vec3(bounds.max.x, bounds.min.y, bounds.min.z);
    corners[2] = glm::vec3(bounds.min.x, bounds.max.y, bounds.min.z);
    corners[3] = glm::vec3(bounds.max.x, bounds.max.y, bounds.min.z);
    corners[4] = glm::vec3(bounds.min.x, bounds.min.y, bounds.max.z);
    corners[5] = glm::vec3(bounds.max.x, bounds.min.y, bounds.max.z);
    corners[6] = glm::vec3(bounds.min.x, bounds.max.y, bounds.max.z);
    corners[7] = glm::vec3(bounds.max.x, bounds.max.y, bounds.max.z);

    glm::vec3 transformedMin = glm::vec3(matrix * glm::vec4(corners[0], 1.0f));
    glm::vec3 transformedMax = transformedMin;
    for (int i = 1; i < 8; i++)
    {
        glm::vec3 transformedCorner = glm::vec3(matrix * glm::vec4(corners[i], 1.0f));
        transformedMin = glm::min(transformedMin, transformedCorner);
        transformedMax = glm::max(transformedMax, transformedCorner);
    }
    Bounds result;
    result.min = transformedMin;
    result.max = transformedMax;
	return result;
}

void AABB::Update(const glm::mat4& worldMat)
{
    this->previousWorldBounds = this->currentWorldBounds;
    this->currentWorldBounds = TransformBounds(this->localBounds, worldMat);
}
