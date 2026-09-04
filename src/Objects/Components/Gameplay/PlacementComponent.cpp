#include "PlacementComponent.hpp"

#include "HierarchyManager.hpp"
#include "HierarchyObject.hpp"
#include "ObjectFactory.hpp"

#include "Components/Camera.hpp"
#include "Components/Transform.hpp"

#include "PropertyDrawers/event_drawer.hpp"

#include "imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

PlacementComponent::PlacementComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("PlacementComponent", owner) {
    GBE_Init();
}

PlacementComponent::~PlacementComponent() noexcept {
    DestroyPlacedVisual();
}

void PlacementComponent::OnUpdate(float /*deltaTime*/) {
    if (!m_hasStoredItem) {
        m_isUnlocked = false;
        DestroyPlacedVisual();
        return;
    }

    if (EnsurePlacedVisual()) {
        SyncPlacedVisualTransform();
    }

    const bool hasRequiredItem = !m_requiredItemId.empty();
    const bool shouldUnlock = hasRequiredItem && m_storedItemId == m_requiredItemId;

    if (shouldUnlock && !m_isUnlocked) {
        m_isUnlocked = true;
        OnUnlock();
    }
    else if (!shouldUnlock && m_isUnlocked) {
        m_isUnlocked = false;
    }
}

void PlacementComponent::OnGUI_ReleaseEvent(float /*deltaTime*/) {
    DrawPlacementOverlay(false);
}

void PlacementComponent::OnGUI_EditorEvent(float /*deltaTime*/) {
    
}

bool PlacementComponent::CanInteract(const glm::vec3& cameraPos, const glm::vec3& cameraForward, float& outDistance) const {
    Transform* anchorTransform = nullptr;
    if (!GetAnchorTransform(anchorTransform) || !anchorTransform) {
        return false;
    }

    const glm::vec3 targetPos = GetAnchorPosition();
    const glm::vec3 toTarget = targetPos - cameraPos;
    const float distance = glm::length(toTarget);

    if (distance <= 0.0001f || distance > m_maxInteractionDistance) {
        return false;
    }

    const glm::vec3 directionToTarget = toTarget / distance;
    const float dot = glm::dot(cameraForward, directionToTarget);
    if (dot < m_facingThreshold) {
        return false;
    }

    outDistance = distance;
    return true;
}

bool PlacementComponent::TryPlaceItem(const std::string& itemId) {
    if (itemId.empty() || m_hasStoredItem) {
        return false;
    }

    m_storedItemId = itemId;
    m_hasStoredItem = true;

    (void)EnsurePlacedVisual();
    return true;
}

bool PlacementComponent::TryTakeItem(std::string& outItemId) {
    if (!m_hasStoredItem) {
        return false;
    }

    outItemId = m_storedItemId;
    m_storedItemId.clear();
    m_hasStoredItem = false;

    DestroyPlacedVisual();
    return true;
}

bool PlacementComponent::GetAnchorTransform(Transform*& outTransform) const {
    outTransform = nullptr;
    if (HierarchyObject::Ref owner = GetOwner()) {
        if (HierarchyObject* ownerPtr = owner.GetPtr()) {
            outTransform = ownerPtr->GetTransform();
            return outTransform != nullptr;
        }
    }

    return false;
}

glm::vec3 PlacementComponent::GetAnchorPosition() const {
    Transform* anchorTransform = nullptr;
    if (!GetAnchorTransform(anchorTransform) || !anchorTransform) {
        return glm::vec3(0.0f);
    }

    return anchorTransform->GetPosition() + glm::vec3(0.0f, m_placementHeight, 0.0f);
}

bool PlacementComponent::EnsurePlacedVisual() {
    HierarchyObject* visualObject = m_placedVisualObject.GetPtr();
    if (!visualObject) {
        m_placedVisualObject = ObjectFactory::GetInstance().CreateCubePrimitive("Placed Item");
        visualObject = m_placedVisualObject.GetPtr();
    }

    if (!visualObject) {
        return false;
    }

    Transform* visualTransform = visualObject->GetTransform();
    if (visualTransform) {
        visualTransform->SetScale(glm::vec3(0.35f));
    }

    return true;
}

void PlacementComponent::DestroyPlacedVisual() {
    if (m_placedVisualObject) {
        HierarchyManager::GetInstance().QueueObjectDeletion(m_placedVisualObject);
        m_placedVisualObject = nullptr;
    }
}

void PlacementComponent::SyncPlacedVisualTransform() const {
    HierarchyObject* visualObject = m_placedVisualObject.GetPtr();
    if (!visualObject) {
        return;
    }

    Transform* anchorTransform = nullptr;
    Transform* visualTransform = visualObject->GetTransform();
    if (!GetAnchorTransform(anchorTransform) || !anchorTransform || !visualTransform) {
        return;
    }

    visualTransform->SetPosition(GetAnchorPosition());
    visualTransform->SetRotation(anchorTransform->GetRotation());
}

void PlacementComponent::DrawPlacementOverlay(bool isEditorContext) {
    CameraComponent* activeCamera = isEditorContext
        ? static_cast<CameraComponent*>(HierarchyManager::GetInstance().GetEditorCamera())
        : HierarchyManager::GetInstance().GetMainCamera();

    if (!activeCamera) {
        return;
    }

    Transform* cameraTransform = nullptr;
    if (HierarchyObject::Ref cameraOwner = activeCamera->GetOwner()) {
        cameraTransform = cameraOwner.GetPtr() ? cameraOwner.GetPtr()->GetTransform() : nullptr;
    }
    if (!cameraTransform) {
        return;
    }

    const glm::vec3 anchorPos = GetAnchorPosition();

    ImVec2 screenPos(0.0f, 0.0f);
    if (!ProjectWorldToScreen(anchorPos, *activeCamera, screenPos)) {
        return;
    }

    const glm::vec3 cameraPos = cameraTransform->GetPosition();
    const glm::vec3 cameraForward = cameraTransform->GetRotation() * glm::vec3(0.0f, 0.0f, 1.0f);
    float distance = 0.0f;
    const bool canLookInteract = CanInteract(cameraPos, cameraForward, distance);
    const bool canPickUpNow = canLookInteract && m_hasStoredItem;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImU32 ringColor = canPickUpNow
        ? IM_COL32(65, 220, 120, 220)
        : IM_COL32(230, 200, 80, 220);
    drawList->AddCircle(screenPos, 10.0f, ringColor, 24, 2.0f);

    ImGui::SetNextWindowBgAlpha(0.78f);
    ImGui::SetNextWindowPos(ImVec2(screenPos.x + 14.0f, screenPos.y - 42.0f), ImGuiCond_Always);
    ImGui::PushID(this);

    if (ImGui::Begin(isEditorContext ? "Placement Debug (Editor)" : "Placement Debug (Release)", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav)) {
        ImGui::Text("Anchor Y Offset: %.2f", m_placementHeight);
        ImGui::Text("Stored Item: %s", m_hasStoredItem ? m_storedItemId.c_str() : "(empty)");
        ImGui::Text("Player Look-At: %s", canLookInteract ? "Yes" : "No");
        ImGui::Text("Player Can Pick Up: %s", canPickUpNow ? "Yes" : "No");
    }
    ImGui::End();
    ImGui::PopID();
}

bool PlacementComponent::ProjectWorldToScreen(const glm::vec3& worldPos, CameraComponent& camera, ImVec2& outScreenPos) const {
    camera.UpdateViewMatrix();
    camera.UpdateProjectionMatrix();

    const glm::mat4 vp = camera.GetProjectionMatrix() * camera.GetViewMatrix();
    const glm::vec4 clip = vp * glm::vec4(worldPos, 1.0f);

    if (clip.w <= 0.0001f) {
        return false;
    }

    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.x < -1.2f || ndc.x > 1.2f || ndc.y < -1.2f || ndc.y > 1.2f || ndc.z < -0.1f || ndc.z > 1.1f) {
        return false;
    }

    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    outScreenPos.x = (ndc.x * 0.5f + 0.5f) * displaySize.x;
    outScreenPos.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * displaySize.y;
    return true;
}

void PlacementComponent::GBE_Init() {
}

void PlacementComponent::OnUnlock() {
    std::cout << "PlacementComponent: Required item placed!" << std::endl;
    m_onUnlockCallback.Invoke();
}
