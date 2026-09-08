#include "EditorCamera.hpp"

#include <algorithm>

#include "HierarchyObject.hpp"
#include "GUIManager.hpp"
#include "imgui.h"

EditorCamera::EditorCamera(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : CameraComponent(transform, owner)
{
    m_name = "EditorCamera";
}

void EditorCamera::OnGUI_EditorEvent(float deltaTime)
{
    (void)deltaTime;

    // Keep input ownership deterministic when multiple editor cameras exist.
    if (IInstanceManager<EditorCamera>::getOldest() != this) {
        return;
    }

    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) {
        return;
    }

    Transform* transform = owner->GetTransform();
    if (!transform) {
        return;
    }

    const auto& gui = Diligent::GUIManager::GetInstance();
    const ImVec2 viewportPos = gui.GetEditorViewportPos();
    const ImVec2 viewportSize = gui.GetEditorViewportSize();
    if (viewportSize.x <= 1.0f || viewportSize.y <= 1.0f) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 viewportMax(viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y);
    const bool mouseInViewport =
        io.MousePos.x >= viewportPos.x && io.MousePos.x <= viewportMax.x &&
        io.MousePos.y >= viewportPos.y && io.MousePos.y <= viewportMax.y;

    static bool s_MiddleDragCapturedByViewport = false;
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle) && mouseInViewport) {
        s_MiddleDragCapturedByViewport = true;
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        s_MiddleDragCapturedByViewport = false;
    }

    const bool viewportInteractive =
        gui.IsEditorViewportHovered() || mouseInViewport || s_MiddleDragCapturedByViewport;
    if (!viewportInteractive) {
        return;
    }

    const bool allowScroll = gui.IsEditorViewportHovered() || mouseInViewport;
    const bool allowDrag = gui.IsEditorViewportFocused() || s_MiddleDragCapturedByViewport;

    glm::vec3 position = transform->GetPosition();
    glm::vec3 euler = transform->GetEulerAnglesDegrees();

    const glm::quat rotation = glm::quat(glm::radians(euler));
    const glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
    const glm::vec3 right = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

    // Scroll wheel = dolly forward/backward along camera forward axis.
    constexpr float kScrollMoveSpeed = 2.5f;
    if (allowScroll && io.MouseWheel != 0.0f) {
        position += forward * (io.MouseWheel * kScrollMoveSpeed);
    }

    if (allowDrag && ImGui::IsMouseDown(ImGuiMouseButton_Middle) && (mouseInViewport || s_MiddleDragCapturedByViewport)) {
        const bool isShiftDown = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);

        if (isShiftDown) {
            // Shift + middle drag pans on local right/up axes.
            constexpr float kPanSpeed = 0.02f;
            position += (-io.MouseDelta.x * kPanSpeed) * right;
            position += (io.MouseDelta.y * kPanSpeed) * up;
        }
        else {
            // Middle drag rotates (orbit-free FPS style yaw/pitch).
            constexpr float kRotateSpeed = 0.15f;
            euler.x = std::clamp(euler.x + io.MouseDelta.y * kRotateSpeed, -89.0f, 89.0f);
            euler.y += io.MouseDelta.x * kRotateSpeed;
            transform->SetEulerAnglesDegrees(euler);
        }
    }

    transform->SetPosition(position);
}