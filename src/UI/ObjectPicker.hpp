#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <iostream>
#include <imgui.h>

#include "Graphics/GraphicsEngine/interface/GraphicsTypes.h" // For Diligent::Uint32
#include "../Rendering/RenderData.hpp"

class ObjectPicker {
public:
    // Delete constructors and assignment operators to enforce a static-only class
    ObjectPicker() = delete;
    ObjectPicker(const ObjectPicker&) = delete;
    ObjectPicker& operator=(const ObjectPicker&) = delete;

    // Define the Ray structure used for picking
    struct Ray {
        glm::vec3 Origin;
        glm::vec3 Direction;
    };

    /**
     * @brief Converts a 2D screen coordinate into a 3D World Space Ray.
     */
    static Ray CreateWorldRayFromScreen(
        const ImVec2& mousePos,
        float screenWidth,
        float screenHeight,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix);

    /**
     * @brief Tests if a Ray intersects an Axis-Aligned Bounding Box (AABB).
     * @return True if intersected, with outDistance populated.
     */
    static bool RayIntersectsAABB(
        const Ray& ray,
        const glm::vec3& boxMin,
        const glm::vec3& boxMax,
        float& outDistance);

    /**
     * @brief Processes left clicks and returns the ID of the clicked object.
     * @return The uint64_t ID of the clicked object, or 0 if nothing was clicked.
     */
    static uint64_t ProcessObjectPicking(
        const RenderData& renderData,
        float localMouseX,
        float localMouseY,
        float viewportWidth,
        float viewportHeight);
};