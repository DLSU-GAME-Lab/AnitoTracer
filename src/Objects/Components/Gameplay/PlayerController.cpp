#include "PlayerController.hpp"

#include "HierarchyManager.hpp"
#include "ObjectFactory.hpp"

#include "Components/Camera.hpp"
#include "Components/Transform.hpp"
#include "PickupComponent.hpp"
#include "PlacementComponent.hpp"

#include "Example/PlayerInput.hpp"
#include "imgui.h"

#include <functional>
#include <limits>

namespace {
    void ForEachObjectRecursive(HierarchyObject::Ref objectRef, const std::function<void(HierarchyObject*)>& fn) {
        HierarchyObject* object = objectRef.GetPtr();
        if (!object) {
            return;
        }

        fn(object);

        for (const auto& child : object->GetChildren()) {
            if (child) {
                ForEachObjectRecursive(child.get(), fn);
            }
        }
    }
}

PlayerController::PlayerController(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("PlayerController", owner), m_transform(transform) {
    GBE_Init();
}

PlayerController::~PlayerController() noexcept {
    DestroyHandVisual();
}

void PlayerController::OnUpdate(float /*deltaTime*/) {
    CameraComponent* mainCamera = HierarchyManager::GetInstance().GetMainCamera();
    if (!mainCamera) {
        return;
    }

    Transform* cameraTransform = nullptr;
    if (HierarchyObject::Ref cameraOwner = mainCamera->GetOwner()) {
        cameraTransform = cameraOwner.GetPtr() ? cameraOwner.GetPtr()->GetTransform() : nullptr;
    }
    if (!cameraTransform) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    if (io.MouseWheel > 0.01f) {
        m_selectedSlot = (m_selectedSlot + 3) % static_cast<int>(m_inventory.size());
    }
    else if (io.MouseWheel < -0.01f) {
        m_selectedSlot = (m_selectedSlot + 1) % static_cast<int>(m_inventory.size());
    }

    const glm::vec3 cameraPos = cameraTransform->GetPosition();
    const glm::vec3 cameraForward = cameraTransform->GetRotation() * glm::vec3(0.0f, 0.0f, 1.0f);

    const PlacementCandidate placementCandidate = FindBestPlacementCandidate(cameraPos, cameraForward);
    const PickupCandidate pickupCandidate = FindBestPickupCandidate(cameraPos, cameraForward);

    if (m_primaryPressedThisFrame) {
        m_primaryPressedThisFrame = false;

        bool consumed = false;
        if (placementCandidate.component) {
            consumed = TryPlaceOrTakeWithSelectedSlot(placementCandidate);
        }

        if (!consumed && pickupCandidate.component) {
            (void)TryPickupIntoSelectedSlot(pickupCandidate);
        }
    }

    const bool selectedSlotHasItem = m_inventory[static_cast<size_t>(m_selectedSlot)].occupied;
    UpdateHandVisual(selectedSlotHasItem);
}

void PlayerController::OnGUI_ReleaseEvent(float /*deltaTime*/) {
    DrawHotbar();
}

void PlayerController::ForEachObject(const std::function<void(HierarchyObject*)>& fn) {
    const auto& roots = HierarchyManager::GetInstance().GetRootObjects();
    for (const auto& root : roots) {
        if (root) {
            ForEachObjectRecursive(root.get(), fn);
        }
    }
}

PlayerController::PickupCandidate PlayerController::FindBestPickupCandidate(const glm::vec3& cameraPos, const glm::vec3& cameraForward) const {
    PickupCandidate best;
    best.distance = std::numeric_limits<float>::max();

    ForEachObject([&](HierarchyObject* object) {
        PickupComponent* pickup = object->GetComponent<PickupComponent>();
        if (!pickup) {
            return;
        }

        Transform* targetTransform = pickup->ResolveTransform();
        if (!targetTransform) {
            return;
        }

        const glm::vec3 targetPos = targetTransform->GetPosition();
        const glm::vec3 toTarget = targetPos - cameraPos;
        const float distance = glm::length(toTarget);
        if (distance <= 0.0001f || distance > pickup->GetMaxPickupDistance()) {
            return;
        }

        const glm::vec3 directionToTarget = toTarget / distance;
        const float dot = glm::dot(cameraForward, directionToTarget);
        if (dot < pickup->GetFacingThreshold()) {
            return;
        }

        if (distance < best.distance) {
            best.component = pickup;
            best.transform = targetTransform;
            best.distance = distance;
        }
    });

    if (!best.component) {
        best.distance = 0.0f;
    }

    return best;
}

PlayerController::PlacementCandidate PlayerController::FindBestPlacementCandidate(const glm::vec3& cameraPos, const glm::vec3& cameraForward) const {
    PlacementCandidate best;
    best.distance = std::numeric_limits<float>::max();

    ForEachObject([&](HierarchyObject* object) {
        PlacementComponent* placement = object->GetComponent<PlacementComponent>();
        if (!placement) {
            return;
        }

        float distance = 0.0f;
        if (!placement->CanInteract(cameraPos, cameraForward, distance)) {
            return;
        }

        if (distance < best.distance) {
            best.component = placement;
            best.distance = distance;
        }
    });

    if (!best.component) {
        best.distance = 0.0f;
    }

    return best;
}

bool PlayerController::TryPickupIntoSelectedSlot(const PickupCandidate& candidate) {
    if (!candidate.component) {
        return false;
    }

    InventorySlot& selectedSlot = m_inventory[static_cast<size_t>(m_selectedSlot)];
    if (selectedSlot.occupied) {
        return false;
    }

    HierarchyObject::Ref owner = candidate.component->GetOwner();
    if (!owner) {
        return false;
    }

    selectedSlot.occupied = true;
    selectedSlot.itemId = candidate.component->GetItemId();

    HierarchyManager::GetInstance().QueueObjectDeletion(owner);
    return true;
}

bool PlayerController::TryPlaceOrTakeWithSelectedSlot(const PlacementCandidate& candidate) {
    if (!candidate.component) {
        return false;
    }

    InventorySlot& selectedSlot = m_inventory[static_cast<size_t>(m_selectedSlot)];

    if (candidate.component->HasItem()) {
        if (selectedSlot.occupied) {
            return false;
        }

        std::string itemId;
        if (candidate.component->TryTakeItem(itemId)) {
            selectedSlot.occupied = true;
            selectedSlot.itemId = itemId;
            return true;
        }

        return false;
    }

    if (!selectedSlot.occupied) {
        return false;
    }

    if (candidate.component->TryPlaceItem(selectedSlot.itemId)) {
        selectedSlot.occupied = false;
        selectedSlot.itemId.clear();
        return true;
    }

    return false;
}

bool PlayerController::EnsureHandVisual() {
    HierarchyObject* handVisual = m_handVisualObject.GetPtr();
    if (!handVisual) {
        m_handVisualObject = ObjectFactory::GetInstance().CreateCubePrimitive("Held Item");
        handVisual = m_handVisualObject.GetPtr();
    }

    if (!handVisual) {
        return false;
    }

    Transform* visualTransform = handVisual->GetTransform();
    if (visualTransform) {
        visualTransform->SetScale(glm::vec3(0.2f));
    }

    return true;
}

void PlayerController::DestroyHandVisual() {
    if (m_handVisualObject) {
        HierarchyManager::GetInstance().QueueObjectDeletion(m_handVisualObject);
        m_handVisualObject = nullptr;
    }
}

void PlayerController::UpdateHandVisual(bool shouldShow) {
    if (!shouldShow) {
        DestroyHandVisual();
        return;
    }

    Transform* handTransform = m_handTransform.Get();
    if (!handTransform) {
        DestroyHandVisual();
        return;
    }

    if (!EnsureHandVisual()) {
        return;
    }

    HierarchyObject* handVisualObject = m_handVisualObject.GetPtr();
    if (!handVisualObject) {
        return;
    }

    const HierarchyObject::Ref handOwner = handTransform->GetOwner();
    if (!handOwner) {
        DestroyHandVisual();
        return;
    }

    if (handVisualObject->GetParent() != handOwner) {
        if (!HierarchyManager::GetInstance().ReparentObject(m_handVisualObject, handOwner)) {
            return;
        }
    }

    Transform* visualTransform = handVisualObject->GetTransform();
    if (!visualTransform) {
        return;
    }

    visualTransform->SetPosition(glm::vec3(0.0f));
    visualTransform->SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
}

void PlayerController::DrawHotbar() {
    ImGuiIO& io = ImGui::GetIO();

    const float slotWidth = 140.0f;
    const float slotHeight = 34.0f;
    const float totalWidth = (slotWidth * static_cast<float>(m_inventory.size())) + (10.0f * (static_cast<float>(m_inventory.size()) - 1.0f));

    ImGui::SetNextWindowBgAlpha(0.6f);
    ImGui::SetNextWindowPos(
        ImVec2((io.DisplaySize.x - totalWidth) * 0.5f, io.DisplaySize.y - 70.0f),
        ImGuiCond_Always
    );

    if (ImGui::Begin("Player Hotbar", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_AlwaysAutoResize)) {

        for (size_t i = 0; i < m_inventory.size(); ++i) {
            if (i > 0) {
                ImGui::SameLine();
            }

            const bool selected = static_cast<int>(i) == m_selectedSlot;
            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.65f, 0.25f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.72f, 0.30f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.55f, 0.20f, 1.0f));
            }

            const InventorySlot& slot = m_inventory[i];
            const std::string label = slot.occupied
                ? ("Slot " + std::to_string(i + 1) + ": " + slot.itemId)
                : ("Slot " + std::to_string(i + 1) + ": Empty");

            ImGui::Button(label.c_str(), ImVec2(slotWidth, slotHeight));

            if (selected) {
                ImGui::PopStyleColor(3);
            }
        }
    }
    ImGui::End();
}

void PlayerController::GBE_Init() {
    std::string key = INPUTKEY_PRIMARY;
    key.append(":Down");

    SubscribeTo(key, [this](const std::unique_ptr<gbe::EventArgs>&) {
        m_primaryPressedThisFrame = true;
    });
}
