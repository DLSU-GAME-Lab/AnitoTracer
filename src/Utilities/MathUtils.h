#pragma once
#include <cstdint>
#include "OBB/AABB.hpp"

struct DirtyRectU32 { uint32_t minX, minY, maxX, maxY; };

static DirtyRectU32 ProjectWorldAabbToScreenRect(
    const AABB::Bounds& worldAabb,
    const glm::mat4& viewProj,
    uint32_t viewportW,
    uint32_t viewportH)
{
    const glm::vec3 mn = worldAabb.min;
    const glm::vec3 mx = worldAabb.max;

    glm::vec3 c[8] = {
        {mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},
        {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z},
    };

    float minXf = std::numeric_limits<float>::infinity();
    float minYf = std::numeric_limits<float>::infinity();
    float maxXf = -std::numeric_limits<float>::infinity();
    float maxYf = -std::numeric_limits<float>::infinity();

    int valid = 0;
    for (int i = 0; i < 8; ++i)
    {
        glm::vec4 clip = viewProj * glm::vec4(c[i], 1.0f);
        if (clip.w <= 0.00001f) continue;

        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        float px = (ndc.x * 0.5f + 0.5f) * float(viewportW);
        float py = (ndc.y * 0.5f + 0.5f) * float(viewportH);

        minXf = std::min(minXf, px);
        minYf = std::min(minYf, py);
        maxXf = std::max(maxXf, px);
        maxYf = std::max(maxYf, py);
        ++valid;
    }

    auto clampU32 = [](float v, uint32_t lo, uint32_t hi) -> uint32_t {
        if (!(v == v) || std::isinf(v)) return lo;
        if (v < float(lo)) return lo;
        if (v > float(hi)) return hi;
        return uint32_t(v);
        };

    if (valid == 0) return { 0, 0, 0, 0 };

    uint32_t maxW = viewportW ? (viewportW - 1u) : 0u;
    uint32_t maxH = viewportH ? (viewportH - 1u) : 0u;

    uint32_t minX = clampU32(std::floor(minXf), 0u, maxW);
    uint32_t minY = clampU32(std::floor(minYf), 0u, maxH);
    uint32_t maxX = clampU32(std::ceil(maxXf), 0u, maxW);
    uint32_t maxY = clampU32(std::ceil(maxYf), 0u, maxH);

    if (maxX < minX) std::swap(maxX, minX);
    if (maxY < minY) std::swap(maxY, minY);

    return { minX, minY, maxX, maxY };
}
