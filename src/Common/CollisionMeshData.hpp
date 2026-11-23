#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

// Plain CPU-side triangle mesh data, shared between a Model and any Colliders that use it
struct CollisionMeshData {
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
};