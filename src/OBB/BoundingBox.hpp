#pragma once

#include <glm/vec3.hpp>
#include <vector>
#include <array>
#include <limits>
#include <algorithm>

class BoundingBox 
{
public:
    BoundingBox(const glm::vec3& center, const std::vector<glm::vec3>& vertices, 
				const std::array<glm::vec3, 3>& axes);
    ~BoundingBox() {}

public:
    glm::vec3 center;              
    std::array<glm::vec3, 3> axes;  
    glm::vec3 halfExtents;         
};
