#include "Capsule.hpp"

using namespace glm;

void Assets::Capsule::Create(const vec3& center, float radius, float height, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
    const int stacks = 32;
    const int slices = 32;
    const float pi = glm::pi<float>();

    // Compute the height of the cylinder (caps removed)
    float cylinderHeight = height - 2 * radius;
    float halfCylinder = cylinderHeight * 0.5f;

    // Hemisphere tessellation steps
    float stackStep = (pi * 0.5f) / float(stacks);
    float sliceStep = 2 * pi / float(slices);

    // --- Top Hemisphere ---
    // Its bottom row should match the top ring of the cylinder at y = center.y + halfCylinder.
    for (int i = 0; i <= stacks; ++i)
    {
        float phi = i * stackStep;  // 0 to pi/2
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);
        for (int j = 0; j <= slices; ++j)
        {
            float theta = j * sliceStep;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);
            float x = radius * sinPhi * cosTheta;
            float y = radius * cosPhi; // y goes from radius to 0
            float z = radius * sinPhi * sinTheta;
            // Shift the hemisphere so that its bottom (phi=pi/2) is at center.y + halfCylinder.
            vec3 pos = center + vec3(x, y + halfCylinder, z);
            vec3 normal = normalize(vec3(x, y, z));
            float u = float(j) / slices;
            float v = 0.5f * (float(i) / stacks);
            vertices.push_back(Vertex{ pos, normal, vec2(u, v), 0 });
        }
    }

    // --- Cylinder ---
    // Two rings: top at y = center.y + halfCylinder, bottom at y = center.y - halfCylinder.
    int cylinderStartIndex = vertices.size();
    for (int i = 0; i <= slices; ++i)
    {
        float theta = i * sliceStep;
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);
        float x = radius * cosTheta;
        float z = radius * sinTheta;
        vec3 normal = normalize(vec3(cosTheta, 0, sinTheta));
        vec3 posTop = center + vec3(x, halfCylinder, z);
        vec3 posBottom = center + vec3(x, -halfCylinder, z);
        float u = float(i) / slices;
        // Using arbitrary v values that connect with hemisphere UVs
        vertices.push_back(Vertex{ posTop, normal, vec2(u, 0.5f), 0 });
        vertices.push_back(Vertex{ posBottom, normal, vec2(u, 0.0f), 0 });
    }

    // --- Bottom Hemisphere ---
    // Its top row should match the bottom ring of the cylinder at y = center.y - halfCylinder.
    int bottomHemisphereStartIndex = vertices.size();
    for (int i = 0; i <= stacks; ++i)
    {
        float phi = i * stackStep;  // 0 to pi/2
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);
        for (int j = 0; j <= slices; ++j)
        {
            float theta = j * sliceStep;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);
            float x = radius * sinPhi * cosTheta;
            float y = radius * cosPhi; // y goes from radius to 0
            float z = radius * sinPhi * sinTheta;
            // For the bottom hemisphere, flip the y-component and shift downward so that its top is at center.y - halfCylinder.
            vec3 pos = center + vec3(x, -y - halfCylinder, z);
            vec3 normal = normalize(vec3(x, -y, z));
            float u = float(j) / slices;
            float v = 1.0f - 0.5f * (float(i) / stacks);
            vertices.push_back(Vertex{ pos, normal, vec2(u, v), 0 });
        }
    }

    // --- Indices for Top Hemisphere ---
    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < slices; ++j)
        {
            int current = i * (slices + 1) + j;
            int next = current + 1;
            int below = (i + 1) * (slices + 1) + j;
            int belowNext = below + 1;
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(below);
            indices.push_back(next);
            indices.push_back(belowNext);
            indices.push_back(below);
        }
    }

    // --- Indices for Cylinder ---
    // Cylinder has (slices+1) vertices in each ring, two vertices per slice.
    for (int i = 0; i < slices; ++i)
    {
        int topCurrent = cylinderStartIndex + i * 2;
        int bottomCurrent = topCurrent + 1;
        int topNext = cylinderStartIndex + ((i + 1) % (slices + 1)) * 2;
        int bottomNext = topNext + 1;
        indices.push_back(topCurrent);
        indices.push_back(topNext);
        indices.push_back(bottomCurrent);
        indices.push_back(bottomCurrent);
        indices.push_back(topNext);
        indices.push_back(bottomNext);
    }

    // --- Indices for Bottom Hemisphere ---
    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < slices; ++j)
        {
            int current = bottomHemisphereStartIndex + i * (slices + 1) + j;
            int next = current + 1;
            int below = bottomHemisphereStartIndex + (i + 1) * (slices + 1) + j;
            int belowNext = below + 1;
            indices.push_back(current);
            indices.push_back(below);
            indices.push_back(next);
            indices.push_back(next);
            indices.push_back(below);
            indices.push_back(belowNext);
        }
    }
}
