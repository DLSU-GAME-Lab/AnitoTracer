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
    const glm::vec3 mn = bounds.min;
    const glm::vec3 mx = bounds.max;

    glm::vec3 corners[8] = {
        {mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},
        {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z},
    };

    Bounds out;
    out.min = glm::vec3(std::numeric_limits<float>::infinity());
    out.max = glm::vec3(-std::numeric_limits<float>::infinity());

    for (int i = 0; i < 8; ++i)
    {
        glm::vec3 p = glm::vec3(matrix * glm::vec4(corners[i], 1.0f));
        out.min = glm::min(out.min, p);
        out.max = glm::max(out.max, p);
    }
    return out;
}

void AABB::Update(const glm::mat4& worldMat)
{
    this->previousWorldBounds = this->currentWorldBounds;
    this->currentWorldBounds = TransformBounds(this->localBounds, worldMat);
}
