#include "EditorCamera.hpp"

#include <algorithm>
#include <cmath>

#include "HierarchyObject.hpp"
#include "GUIManager.hpp"
#include "imgui.h"

EditorCamera::EditorCamera(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : CameraComponent(transform, owner)
{}

void EditorCamera::FocusOn(const glm::vec3& position)
{
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) {
        return;
    }

    Transform* transform = owner->GetTransform();
    if (!transform) {
        return;
    }

    m_pivotPosition = position;
    m_distance = kDefaultDistance;

    const glm::vec3 euler = transform->GetEulerAnglesDegrees();
    const glm::quat rotation = glm::quat(glm::radians(euler));
    const glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);

    const glm::vec3 newPosition = m_pivotPosition - forward * m_distance;
    transform->SetPosition(newPosition);
}

void EditorCamera::OnGUI_EditorEvent(float deltaTime)
{
    (void)deltaTime;

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

    glm::quat rotation = glm::quat(glm::radians(euler));
    glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 right = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

    m_distance = glm::length(position - m_pivotPosition);

    // Exponential scroll wheel zoom
    if (allowScroll && io.MouseWheel != 0.0f) {
        constexpr float kZoomSensitivity = 0.15f;
        const float zoomFactor = std::exp(-io.MouseWheel * kZoomSensitivity);
        m_distance = std::max(0.01f, m_distance * zoomFactor);
        position = m_pivotPosition - forward * m_distance;
    }

    if (allowDrag && ImGui::IsMouseDown(ImGuiMouseButton_Middle) && (mouseInViewport || s_MiddleDragCapturedByViewport)) {
        const bool isShiftDown = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);

        if (isShiftDown) {
            // Pan speed scales dynamically with distance to maintain 1:1 screen-space tracking
            constexpr float kBasePanSpeed = 0.002f;
            const float dynamicPanSpeed = kBasePanSpeed * m_distance;

            const glm::vec3 panDelta = (-io.MouseDelta.x * dynamicPanSpeed) * right + (io.MouseDelta.y * dynamicPanSpeed) * up;
            position += panDelta;
            m_pivotPosition += panDelta;
        }
        else {
            // Middle drag orbits around the cached pivot position
            constexpr float kRotateSpeed = 0.15f;
            euler.x = std::clamp(euler.x + io.MouseDelta.y * kRotateSpeed, -89.0f, 89.0f);
            euler.y += io.MouseDelta.x * kRotateSpeed;

            rotation = glm::quat(glm::radians(euler));
            forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);

            position = m_pivotPosition - forward * m_distance;
            transform->SetEulerAnglesDegrees(euler);
        }
    }

    transform->SetPosition(position);
}