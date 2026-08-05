#include "ObjectPicker.hpp"


ObjectPicker::Ray ObjectPicker::CreateWorldRayFromScreen(
    const ImVec2& mousePos,
    float screenWidth,
    float screenHeight,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix)
{
    glm::vec4 viewport(0.0f, 0.0f, screenWidth, screenHeight);

    // Flip Y coordinate because ImGui uses top-left origin (0,0) but standard graphics APIs use bottom-left
    glm::vec3 winNear(mousePos.x, screenHeight - mousePos.y, 0.0f);
    glm::vec3 winFar(mousePos.x, screenHeight - mousePos.y, 1.0f);

    glm::vec3 worldNear = glm::unProject(winNear, viewMatrix, projMatrix, viewport);
    glm::vec3 worldFar = glm::unProject(winFar, viewMatrix, projMatrix, viewport);

    Ray ray;
    ray.Origin = worldNear;
    ray.Direction = glm::normalize(worldFar - worldNear);
    return ray;
}

bool ObjectPicker::RayIntersectsAABB(
    const Ray& ray,
    const glm::vec3& boxMin,
    const glm::vec3& boxMax,
    float& outDistance)
{
    glm::vec3 invDir = 1.0f / ray.Direction;
    glm::vec3 t0 = (boxMin - ray.Origin) * invDir;
    glm::vec3 t1 = (boxMax - ray.Origin) * invDir;

    glm::vec3 tMin = glm::min(t0, t1);
    glm::vec3 tMax = glm::max(t0, t1);

    float tNear = std::max({ tMin.x, tMin.y, tMin.z });
    float tFar = std::min({ tMax.x, tMax.y, tMax.z });

    if (tNear <= tFar && tFar > 0.0f) {
        outDistance = tNear > 0.0f ? tNear : tFar;
        return true;
    }
    return false;
}

uint64_t ObjectPicker::ProcessObjectPicking(
    const RenderData& renderData,
    Diligent::Uint32 screenWidth,
    Diligent::Uint32 screenHeight)
{
    ImGuiIO& io = ImGui::GetIO();

    // Only process picking if the user clicked the Left Mouse Button AND ImGui isn't using the mouse
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse)
    {
        ImVec2 mousePos = ImGui::GetMousePos();

        // Generate the 3D ray based on the current camera matrices
        Ray worldRay = CreateWorldRayFromScreen(
            mousePos,
            static_cast<float>(screenWidth),
            static_cast<float>(screenHeight),
            renderData.ViewMatrix,
            renderData.ProjectionMatrix
        );

        uint64_t closestObjectID = 0;
        float closestDistance = std::numeric_limits<float>::max();

        // Loop through all rendered models to check for collisions
        for (const auto& modelInstance : renderData.Models)
        {
            if (!modelInstance.ModelData || modelInstance.OwnerID == 0) continue;

            // Transform the World Ray into the Model's Local Space
            glm::mat4 invWorld = glm::inverse(modelInstance.WorldTransform);

            Ray localRay;
            localRay.Origin = glm::vec3(invWorld * glm::vec4(worldRay.Origin, 1.0f));
            localRay.Direction = glm::normalize(glm::vec3(invWorld * glm::vec4(worldRay.Direction, 0.0f)));

            float hitDistance = 0.0f;
            if (RayIntersectsAABB(localRay, modelInstance.ModelData->AABBMin, modelInstance.ModelData->AABBMax, hitDistance))
            {
                // If this object is closer to the camera than the previous closest hit, select it
                if (hitDistance < closestDistance)
                {
                    closestDistance = hitDistance;
                    closestObjectID = modelInstance.OwnerID;
                }
            }
        }

        return closestObjectID;
    }

    // Return 0 if there was no click, or no object was hit
    return 0;
}