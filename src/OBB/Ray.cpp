#include "Ray.hpp"

#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>

Ray::Ray(const glm::vec3& origin, const glm::vec3& direction)
	: origin(origin), direction(glm::normalize(direction))
{
}

bool Ray::intersects(const BoundingBox& box, float& tOut) const
{
    // Using the slab method for each local axis.
    float tMin = -std::numeric_limits<float>::max();
    float tMax = std::numeric_limits<float>::max();

    // Vector from ray origin to box center.
    glm::vec3 p = box.center - origin;

    // Check intersection against each of the box's axes.
    for (int i = 0; i < 3; ++i) {
        // Project the vector from the ray origin to the center onto the i-th axis.
        float e = glm::dot(box.axes[i], p);
        // Project the ray direction onto the i-th axis.
        float f = glm::dot(box.axes[i], direction);

        if (std::fabs(f) > 1e-6f) {
            // Calculate intersection distances along the ray for the slab.
            float t1 = (e + box.halfExtents[i]) / f;
            float t2 = (e - box.halfExtents[i]) / f;
            if (t1 > t2)
                std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            if (tMin > tMax)
                return false;
        }
        else {
            // Ray is parallel to slab. If origin is not within slab, no intersection.
            if (-e - box.halfExtents[i] > 0 || -e + box.halfExtents[i] < 0)
                return false;
        }
    }

    if (tMax < 0)
        return false;

    tOut = (tMin >= 0.0f) ? tMin : tMax;
    return true;
}


glm::vec3 Ray::getOrigin() const
{
    return this->origin;
}

glm::vec3 Ray::getDirection() const
{
    return this->direction;
}
