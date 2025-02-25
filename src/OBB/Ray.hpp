#pragma once

#include <glm/vec3.hpp>
#include "BoundingBox.hpp"

class Ray
{
public:
    Ray(const glm::vec3& origin, const glm::vec3& direction);

    bool intersects(const BoundingBox& box, float& t) const;

    glm::vec3 getOrigin() const;
    glm::vec3 getDirection() const;

private:
    glm::vec3 origin;
    glm::vec3 direction; 
};
