#include "AnimeMeshData.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <unordered_map>

namespace {

    glm::vec3 ToGlm(const float3& v) { return glm::vec3(v.x, v.y, v.z); }

    // A position snapped to a grid, the key vertices are welded by
    struct WeldKey {
        int64_t x, y, z;
        bool operator==(const WeldKey& other) const { return x == other.x && y == other.y && z == other.z; }
    };

    struct WeldKeyHash {
        size_t operator()(const WeldKey& key) const {
            uint64_t h = static_cast<uint64_t>(key.x) * 0x9E3779B97F4A7C15ull;
            h ^= static_cast<uint64_t>(key.y) + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
            h ^= static_cast<uint64_t>(key.z) + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
            return static_cast<size_t>(h);
        }
    };

    glm::vec3 NormalizeOr(const glm::vec3& v, const glm::vec3& fallback) {
        const float length = glm::length(v);
        return length > 1e-6f ? v / length : fallback;
    }

}

std::vector<AnimeVertexData> BuildAnimeVertexData(const std::vector<Vertex>& vertices, const std::vector<Uint32>& indices) {
    const size_t vertexCount = vertices.size();
    std::vector<AnimeVertexData> result(vertexCount);
    if (vertexCount == 0) {
        return result;
    }

    // ---- 1. Weld vertices that sit at the same position ----------------------------------------------------------
    glm::vec3 boundsMin(FLT_MAX);
    glm::vec3 boundsMax(-FLT_MAX);
    for (const Vertex& vertex : vertices) {
        boundsMin = glm::min(boundsMin, ToGlm(vertex.pos));
        boundsMax = glm::max(boundsMax, ToGlm(vertex.pos));
    }
    // Seams hold bit-identical positions in practice; the grid only forgives export rounding
    const float cell = std::max(glm::length(boundsMax - boundsMin) * 1e-5f, 1e-7f);

    std::vector<uint32_t> weldOf(vertexCount);
    std::unordered_map<WeldKey, uint32_t, WeldKeyHash> welds;
    welds.reserve(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i) {
        const glm::vec3 p = ToGlm(vertices[i].pos);
        const WeldKey key{
            static_cast<int64_t>(std::llround(p.x / cell)),
            static_cast<int64_t>(std::llround(p.y / cell)),
            static_cast<int64_t>(std::llround(p.z / cell)) };
        const uint32_t next = static_cast<uint32_t>(welds.size());
        weldOf[i] = welds.try_emplace(key, next).first->second;
    }
    const size_t weldCount = welds.size();

    // ---- 2. Smooth normal per welded position ------------------------------------------------------------------------
    std::vector<glm::vec3> normalSum(weldCount, glm::vec3(0.0f));
    std::vector<glm::vec3> faceSum(weldCount, glm::vec3(0.0f));
    for (size_t i = 0; i < vertexCount; ++i) {
        normalSum[weldOf[i]] += NormalizeOr(ToGlm(vertices[i].normal), glm::vec3(0.0f));
    }

    // The cross product's length is twice the triangle's area, so larger faces weigh more
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const uint32_t i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
        const glm::vec3 p0 = ToGlm(vertices[i0].pos);
        const glm::vec3 faceNormal = glm::cross(ToGlm(vertices[i1].pos) - p0, ToGlm(vertices[i2].pos) - p0);
        faceSum[weldOf[i0]] += faceNormal;
        faceSum[weldOf[i1]] += faceNormal;
        faceSum[weldOf[i2]] += faceNormal;
    }

    // meanNormal is the vertices' own normals averaged, which also says which way the face normals should point
    // (the winding depends on how the model was imported)
    std::vector<glm::vec3> meanNormal(weldCount);
    std::vector<glm::vec3> smoothNormal(weldCount);
    for (size_t w = 0; w < weldCount; ++w) {
        meanNormal[w] = NormalizeOr(normalSum[w], NormalizeOr(faceSum[w], glm::vec3(0.0f, 1.0f, 0.0f)));

        glm::vec3 smooth = NormalizeOr(faceSum[w], meanNormal[w]);
        if (glm::dot(smooth, meanNormal[w]) < 0.0f) {
            smooth = -smooth;
        }
        smoothNormal[w] = smooth;
    }

    // ---- 3. Which positions touch: the unique edges of every triangle, as lists of neighbors per position ----------
    std::vector<uint64_t> edges;
    edges.reserve(indices.size());
    auto addEdge = [&](uint32_t a, uint32_t b) {
        if (a == b) return;
        if (a > b) std::swap(a, b);
        edges.push_back((static_cast<uint64_t>(a) << 32) | b);
    };
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        const uint32_t w0 = weldOf[indices[i]], w1 = weldOf[indices[i + 1]], w2 = weldOf[indices[i + 2]];
        addEdge(w0, w1);
        addEdge(w1, w2);
        addEdge(w2, w0);
    }
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

    std::vector<uint32_t> neighborStart(weldCount + 1, 0);
    for (const uint64_t edge : edges) {
        ++neighborStart[(edge >> 32) + 1];
        ++neighborStart[(edge & 0xFFFFFFFFull) + 1];
    }
    for (size_t w = 0; w < weldCount; ++w) {
        neighborStart[w + 1] += neighborStart[w];
    }
    std::vector<uint32_t> neighbors(neighborStart[weldCount]);
    std::vector<uint32_t> fill(neighborStart.begin(), neighborStart.end() - 1);
    for (const uint64_t edge : edges) {
        const uint32_t a = static_cast<uint32_t>(edge >> 32);
        const uint32_t b = static_cast<uint32_t>(edge & 0xFFFFFFFFull);
        neighbors[fill[a]++] = b;
        neighbors[fill[b]++] = a;
    }

    // ---- 4. Curvature ----------------------------------------------------------------------------------------------------
    // Where the vertices at one position disagree about the normal (a hard edge), 1 - dot^2 against their mean is
    // the same measure as (1 - dot) / 2 between the two normals: 0.5 at a right angle
    std::vector<float> spread(weldCount, 0.0f);
    for (size_t i = 0; i < vertexCount; ++i) {
        const glm::vec3 n = NormalizeOr(ToGlm(vertices[i].normal), glm::vec3(0.0f));
        const uint32_t w = weldOf[i];
        const float d = glm::dot(n, meanNormal[w]);
        spread[w] = std::max(spread[w], 1.0f - d * d);
    }

    // Weeb_Big2's raw curvature: the average of (1 - dot) / 2 against the neighbors' normals. 0 flat, 1 opposite.
    std::vector<float> raw(weldCount, 0.0f);
    for (size_t w = 0; w < weldCount; ++w) {
        const uint32_t count = neighborStart[w + 1] - neighborStart[w];
        float average = 0.0f;
        if (count > 0) {
            float sum = 0.0f;
            for (uint32_t k = neighborStart[w]; k < neighborStart[w + 1]; ++k) {
                sum += (1.0f - glm::dot(meanNormal[w], meanNormal[neighbors[k]])) * 0.5f;
            }
            average = sum / static_cast<float>(count);
        }
        raw[w] = std::max(spread[w], average);
    }

    // ...averaged once more with the neighbors, which turns a crease's single vertex into a line a few triangles wide
    std::vector<float> curvature(weldCount, 0.0f);
    for (size_t w = 0; w < weldCount; ++w) {
        const uint32_t count = neighborStart[w + 1] - neighborStart[w];
        if (count == 0) {
            curvature[w] = raw[w];
            continue;
        }
        float sum = 0.0f;
        for (uint32_t k = neighborStart[w]; k < neighborStart[w + 1]; ++k) {
            sum += raw[neighbors[k]];
        }
        curvature[w] = sum / static_cast<float>(count);
    }

    // ---- 5. Back to the vertices -----------------------------------------------------------------------------------------
    for (size_t i = 0; i < vertexCount; ++i) {
        const uint32_t w = weldOf[i];
        result[i].smoothNormal = float3(smoothNormal[w].x, smoothNormal[w].y, smoothNormal[w].z);
        result[i].curvature = glm::clamp(curvature[w], 0.0f, 1.0f);
    }
    return result;
}
