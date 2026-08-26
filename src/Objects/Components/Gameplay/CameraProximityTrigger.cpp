#include "CameraProximityTrigger.hpp"

#include "PropertyDrawers/event_drawer.hpp"

#include "HierarchyManager.hpp"
#include "Components/Transform.hpp"
#include "Components/Camera.hpp"
#include "imgui.h"

#include <iostream>

CameraProximityTrigger::CameraProximityTrigger(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("CameraProximityTrigger", owner), m_transform(transform) {}

CameraProximityTrigger::~CameraProximityTrigger() = default;

void CameraProximityTrigger::OnUpdate(float /*deltaTime*/) {
    // 1. Fetch main camera and target object transform
    CameraComponent* mainCamera = HierarchyManager::GetInstance().GetMainCamera();
    if (!mainCamera) return;

    Transform* objTransform = m_transform;
    if (!objTransform && GetOwner().GetPtr()) {
        objTransform = GetOwner().GetPtr()->GetTransform();
    }
    if (!objTransform) return;

    Transform* cameraTransform = mainCamera->GetOwner().GetPtr()
        ? mainCamera->GetOwner().GetPtr()->GetTransform()
        : nullptr;
    if (!cameraTransform) return;

    // 2. Measure distance between main camera and this object
    glm::vec3 cameraPos = cameraTransform->GetPosition();
    glm::vec3 objectPos = objTransform->GetPosition();
    float distance = glm::distance(cameraPos, objectPos);

    // 3. Proximity state update and trigger invocation
    bool currentlyInRange = (distance <= m_triggerDistance);

    if (currentlyInRange && !m_isInRange) {
        m_isInRange = true;
        OnProximityEnter();
    }
    else if (!currentlyInRange && m_isInRange) {
        m_isInRange = false;
        m_waitingForConfirmation = false;
    }

    if (m_waitingForConfirmation) {
        DrawConfirmationPrompt();

        if (ImGui::IsKeyPressed(ImGuiKey_Enter, false)) {
            m_waitingForConfirmation = false;
            m_onProximityCallback.Invoke();
        }
    }
}

void CameraProximityTrigger::OnProximityEnter() {
    std::cout << "CameraProximityTrigger: Camera entered threshold distance!" << std::endl;
    m_waitingForConfirmation = true;
}

void CameraProximityTrigger::DrawConfirmationPrompt() {
    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
    ImGui::PushID(this);

    if (ImGui::Begin("Interaction Prompt", nullptr,
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoFocusOnAppearing)) {
        const char* objectName = "object";
        if (HierarchyObject::Ref owner = GetOwner()) {
            objectName = owner.GetPtr()->GetName().c_str();
        }

        ImGui::Text("Press Enter to interact with %s", objectName);
    }
    ImGui::End();
    ImGui::PopID();
}
