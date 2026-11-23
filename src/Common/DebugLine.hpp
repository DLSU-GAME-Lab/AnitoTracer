#pragma once
#include <glm/glm.hpp>

struct DebugLineVertex {
    glm::vec3 Pos;
    glm::vec4 Color;
};

inline constexpr size_t kMaxDebugLineVertices = ((4u << 20) / sizeof(DebugLineVertex)) & ~size_t(1);