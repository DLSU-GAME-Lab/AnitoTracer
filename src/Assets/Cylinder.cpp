#include "Cylinder.hpp"

using namespace glm;

void Assets::Cylinder::Create(const vec3& center, float radius, float height, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
    const int slices = 32;
    const float pi = 3.14159265358979f;
    const float halfHeight = height * 0.5f;
    float sliceStep = 2 * pi / static_cast<float>(slices);

    int topCenterIndex = 0;
    for (int i = 1; i <= slices; ++i)
    {
        int nextIndex = (i % slices) + 1;
        indices.push_back(topCenterIndex);
        indices.push_back(nextIndex);
        indices.push_back(i);
    }

    unsigned int bottomCenterIndex = slices + 1;
    for (unsigned int i = 0; i < slices; ++i)
    {
        unsigned int currentVertexIndex = bottomCenterIndex + 1 + i;
        unsigned int nextVertexIndex = bottomCenterIndex + 1 + ((i + 1) % slices);

        indices.push_back(bottomCenterIndex);
        indices.push_back(currentVertexIndex);
        indices.push_back(nextVertexIndex);
    }

    int sideStartIndex = bottomCenterIndex + slices + 1;
    for (int i = 0; i < slices; ++i)
    {
        int topIndex = sideStartIndex + i * 2;
        int bottomIndex = topIndex + 1;
        int nextTopIndex = sideStartIndex + ((i + 1) % slices) * 2;
        int nextBottomIndex = nextTopIndex + 1;

        indices.push_back(topIndex);
        indices.push_back(nextTopIndex);
        indices.push_back(bottomIndex);

        indices.push_back(bottomIndex);
        indices.push_back(nextTopIndex);
        indices.push_back(nextBottomIndex);
    }

    Vertex topCenter = {
        glm::vec3(0.0f, halfHeight, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec2(0.5f, 0.5f),
        0
    };
    vertices.push_back(topCenter);

    // Top rim vertices
    for (unsigned int i = 0; i < slices; ++i)
    {
        float theta = i * sliceStep;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);

        // For the top cap, the normal is up
        glm::vec3 normal(0.0f, 1.0f, 0.0f);

        // UV coordinates mapped to [0,1] with center at 0.5
        glm::vec2 uv((x / radius + 1.0f) * 0.5f, (z / radius + 1.0f) * 0.5f);
        Vertex topVertex = {
            glm::vec3(x, halfHeight, z),
            normal,
            uv,
            0
        };
        vertices.push_back(topVertex);
    }

    // 2. Bottom Cap
    // Bottom center vertex (normal pointing down)
    Vertex bottomCenter = {
        glm::vec3(0.0f, -halfHeight, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec2(0.5f, 0.5f),
        0
    };
    vertices.push_back(bottomCenter);

    // Bottom rim vertices
    for (unsigned int i = 0; i < slices; ++i)
    {
        float theta = i * sliceStep;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);

        // For the bottom cap, the normal is down
        glm::vec3 normal(0.0f, -1.0f, 0.0f);

        // UV coordinates mapped similarly to the top cap
        glm::vec2 uv((x / radius + 1.0f) * 0.5f, (z / radius + 1.0f) * 0.5f);
        Vertex bottomVertex = {
            glm::vec3(x, -halfHeight, z),
            normal,
            uv,
            0
        };
        vertices.push_back(bottomVertex);
    }

    // 3. Side Wall
    // For the sides, compute normals from the x and z components.
    for (unsigned int i = 0; i < slices; ++i)
    {
        float theta = i * sliceStep;
        float x = radius * cosf(theta);
        float z = radius * sinf(theta);

        // Normal is the xz-direction (normalized)
        glm::vec3 normal = glm::normalize(glm::vec3(x, 0.0f, z));

        // UVs: U coordinate based on the slice fraction, V coordinate 0 for top and 1 for bottom.
        glm::vec2 uvTop(i / static_cast<float>(slices), 0.0f);
        glm::vec2 uvBottom(i / static_cast<float>(slices), 1.0f);

        // Top vertex of the side wall
        Vertex sideTopVertex = {
            glm::vec3(x, halfHeight, z),
            normal,
            uvTop,
            0
        };
        vertices.push_back(sideTopVertex);

        // Bottom vertex of the side wall
        Vertex sideBottomVertex = {
            glm::vec3(x, -halfHeight, z),
            normal,
            uvBottom,
            0
        };
        vertices.push_back(sideBottomVertex);
    }
}