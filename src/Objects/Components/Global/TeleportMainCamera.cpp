#include "TeleportMainCamera.hpp"
#include <iostream>

#include <glm/glm.hpp>

#include "Components/EditorCamera.hpp"
#include "Components/GameCamera.hpp"
#include "GUIManager.hpp"
#include "HierarchyObject.hpp"
#include "imgui.h"

#include "Organization/IInstanceManager.hpp"

TeleportMainCamera::TeleportMainCamera(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("TeleportMainCamera", owner) {}

void TeleportMainCamera::DoTeleport() {
    gbe::IInstanceManager<GameCamera>::getOldest()->GetOwner().GetPtr()->GetTransform()->SetPosition(this->m_targetpos);
}

void TeleportMainCamera::OnGUI_EditorEvent(float deltaTime)
{
    (void)deltaTime;

    EditorCamera* editorCamera = gbe::IInstanceManager<EditorCamera>::getOldest();
    if (!editorCamera) {
        return;
    }

    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) {
        return;
    }

    Transform* selfTransform = owner->GetTransform();
    if (!selfTransform) {
        return;
    }

    editorCamera->UpdateViewMatrix();
    editorCamera->UpdateProjectionMatrix();

    const glm::mat4 view = editorCamera->GetViewMatrix();
    const glm::mat4 proj = editorCamera->GetProjectionMatrix();
    const glm::mat4 vp = proj * view;

    const auto& gui = Diligent::GUIManager::GetInstance();
    const ImVec2 viewportPos = gui.GetEditorViewportPos();
    const ImVec2 viewportSize = gui.GetEditorViewportSize();
    if (viewportSize.x <= 1.0f || viewportSize.y <= 1.0f) {
        return;
    }

    const auto ProjectToScreen = [&vp, &viewportPos, &viewportSize](const glm::vec3& worldPos, ImVec2& outScreen) -> bool {
        const glm::vec4 clip = vp * glm::vec4(worldPos, 1.0f);
        if (clip.w <= 0.0001f) {
            return false;
        }

        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        if (ndc.z < 0.0f || ndc.z > 1.0f) {
            return false;
        }

        outScreen.x = viewportPos.x + (ndc.x * 0.5f + 0.5f) * viewportSize.x;
        outScreen.y = viewportPos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * viewportSize.y;
        return true;
    };

    ImVec2 selfPosScreen{};
    ImVec2 targetPosScreen{};
    if (!ProjectToScreen(selfTransform->GetPosition(), selfPosScreen) ||
        !ProjectToScreen(m_targetpos, targetPosScreen)) {
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    constexpr ImU32 kLineColor = IM_COL32(90, 210, 255, 230);
    constexpr float kThickness = 2.0f;
    drawList->AddLine(selfPosScreen, targetPosScreen, kLineColor, kThickness);
}
