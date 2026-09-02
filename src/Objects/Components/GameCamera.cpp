#include "GameCamera.hpp"

#include <array>
#include <cmath>

#include <glm/glm.hpp>

#include "EditorCamera.hpp"
#include "HierarchyObject.hpp"
#include "imgui.h"

GameCamera::GameCamera(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : CameraComponent(transform, owner)
{
    m_name = "GameCamera";
}

void GameCamera::OnGUI_EditorEvent(float deltaTime)
{
    (void)deltaTime;

    // Draw marker only for the primary game camera used as the runtime camera.
    if (IInstanceManager<GameCamera>::getOldest() != this) {
        return;
    }

    EditorCamera* editorCamera = IInstanceManager<EditorCamera>::getOldest();
    if (!editorCamera) {
        return;
    }

    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) {
        return;
    }

    Transform* gameCamTransform = owner->GetTransform();
    if (!gameCamTransform) {
        return;
    }

    editorCamera->UpdateViewMatrix();
    editorCamera->UpdateProjectionMatrix();

    const glm::mat4 view = editorCamera->GetViewMatrix();
    const glm::mat4 proj = editorCamera->GetProjectionMatrix();
    const glm::mat4 vp = proj * view;

    const glm::vec3 gameCamPos = gameCamTransform->GetPosition();
    const glm::quat gameCamRot = gameCamTransform->GetRotation();
    const glm::vec3 gameForward = gameCamRot * glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 gameRight = gameCamRot * glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 gameUp = gameCamRot * glm::vec3(0.0f, 1.0f, 0.0f);

    constexpr float kFulcrumDistance = 3.0f;
    const glm::vec3 fulcrumWorldPos = gameCamPos + gameForward * kFulcrumDistance;

    const auto ProjectToScreen = [&vp](const glm::vec3& worldPos, ImVec2& outScreen) -> bool {
        const glm::vec4 clip = vp * glm::vec4(worldPos, 1.0f);
        if (clip.w <= 0.0001f) {
            return false;
        }

        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.z < 0.0f || ndc.z > 1.0f) {
            return false;
        }

        const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        outScreen.x = (ndc.x * 0.5f + 0.5f) * displaySize.x;
        outScreen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * displaySize.y;
        return true;
    };

    ImVec2 camPosScreen{};
    ImVec2 fulcrumPosScreen{};
    if (!ProjectToScreen(gameCamPos, camPosScreen) || !ProjectToScreen(fulcrumWorldPos, fulcrumPosScreen)) {
        return;
    }

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    constexpr ImU32 kLineColor = IM_COL32(255, 180, 60, 230);
    constexpr ImU32 kNearPlaneColor = IM_COL32(80, 220, 140, 220);
    constexpr ImU32 kFarPlaneColor = IM_COL32(80, 140, 255, 220);
    constexpr float kThickness = 2.0f;
    constexpr float kCrossSize = 8.0f;

    drawList->AddLine(camPosScreen, fulcrumPosScreen, kLineColor, kThickness);
    drawList->AddLine(
        ImVec2(fulcrumPosScreen.x - kCrossSize, fulcrumPosScreen.y),
        ImVec2(fulcrumPosScreen.x + kCrossSize, fulcrumPosScreen.y),
        kLineColor,
        kThickness
    );
    drawList->AddLine(
        ImVec2(fulcrumPosScreen.x, fulcrumPosScreen.y - kCrossSize),
        ImVec2(fulcrumPosScreen.x, fulcrumPosScreen.y + kCrossSize),
        kLineColor,
        kThickness
    );

    const float nearZ = std::max(GetNearPlane(), 0.001f);
    const float farZ = std::max(GetFarPlane(), nearZ + 0.001f);
    const float aspect = std::max(GetAspect(), 0.001f);
    const float tanHalfFov = std::tan(glm::radians(GetFOV() * 0.5f));

    const float nearHalfHeight = tanHalfFov * nearZ;
    const float nearHalfWidth = nearHalfHeight * aspect;
    const float farHalfHeight = tanHalfFov * farZ;
    const float farHalfWidth = farHalfHeight * aspect;

    const glm::vec3 nearCenter = gameCamPos + gameForward * nearZ;
    const glm::vec3 farCenter = gameCamPos + gameForward * farZ;

    const std::array<glm::vec3, 4> nearCorners = {
        nearCenter + gameUp * nearHalfHeight - gameRight * nearHalfWidth,
        nearCenter + gameUp * nearHalfHeight + gameRight * nearHalfWidth,
        nearCenter - gameUp * nearHalfHeight + gameRight * nearHalfWidth,
        nearCenter - gameUp * nearHalfHeight - gameRight * nearHalfWidth
    };

    const std::array<glm::vec3, 4> farCorners = {
        farCenter + gameUp * farHalfHeight - gameRight * farHalfWidth,
        farCenter + gameUp * farHalfHeight + gameRight * farHalfWidth,
        farCenter - gameUp * farHalfHeight + gameRight * farHalfWidth,
        farCenter - gameUp * farHalfHeight - gameRight * farHalfWidth
    };

    const auto DrawProjectedSegment = [&ProjectToScreen, drawList](
        const glm::vec3& a,
        const glm::vec3& b,
        ImU32 color,
        float thickness) {
            ImVec2 aScreen{};
            ImVec2 bScreen{};
            if (!ProjectToScreen(a, aScreen) || !ProjectToScreen(b, bScreen)) {
                return;
            }
            drawList->AddLine(aScreen, bScreen, color, thickness);
        };

    // Near plane rectangle.
    DrawProjectedSegment(nearCorners[0], nearCorners[1], kNearPlaneColor, 1.8f);
    DrawProjectedSegment(nearCorners[1], nearCorners[2], kNearPlaneColor, 1.8f);
    DrawProjectedSegment(nearCorners[2], nearCorners[3], kNearPlaneColor, 1.8f);
    DrawProjectedSegment(nearCorners[3], nearCorners[0], kNearPlaneColor, 1.8f);

    // Far plane rectangle.
    DrawProjectedSegment(farCorners[0], farCorners[1], kFarPlaneColor, 1.8f);
    DrawProjectedSegment(farCorners[1], farCorners[2], kFarPlaneColor, 1.8f);
    DrawProjectedSegment(farCorners[2], farCorners[3], kFarPlaneColor, 1.8f);
    DrawProjectedSegment(farCorners[3], farCorners[0], kFarPlaneColor, 1.8f);

    // Frustum side edges.
    DrawProjectedSegment(nearCorners[0], farCorners[0], kLineColor, 1.4f);
    DrawProjectedSegment(nearCorners[1], farCorners[1], kLineColor, 1.4f);
    DrawProjectedSegment(nearCorners[2], farCorners[2], kLineColor, 1.4f);
    DrawProjectedSegment(nearCorners[3], farCorners[3], kLineColor, 1.4f);
}