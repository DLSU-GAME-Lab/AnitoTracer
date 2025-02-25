#include "BoundingBox.hpp"

#include <glm/ext/quaternion_geometric.hpp>

BoundingBox::BoundingBox(const glm::vec3& center, const std::vector<glm::vec3>& vertices, const std::array<glm::vec3, 3>& inAxes)
    : axes(inAxes), center(center)
{
    glm::vec3 sum(0.0f);
    for (const auto& v : vertices) {
        sum += v;
    }

    float minProj[3] = { std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max() };
    float maxProj[3] = { -std::numeric_limits<float>::max(),
                         -std::numeric_limits<float>::max(),
                         -std::numeric_limits<float>::max() };
    for (const auto& v : vertices) {
        glm::vec3 diff = v - center;
        for (int i = 0; i < 3; ++i) {
            float proj = glm::dot(diff, axes[i]);
            minProj[i] = std::min(minProj[i], proj);
            maxProj[i] = std::max(maxProj[i], proj);
        }
    }

    halfExtents = glm::vec3(
        (maxProj[0] - minProj[0]) * 0.5f,
        (maxProj[1] - minProj[1]) * 0.5f,
        (maxProj[2] - minProj[2]) * 0.5f
    );

}
