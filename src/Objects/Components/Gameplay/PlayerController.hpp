#pragma once

#include "Components/ComponentBase.hpp"
#include "EventHandler.hpp"
#include "Types/OnGUI_Release.hpp"
#include "Types/UpdateTrigger.hpp"
#include "ObjectRef.hpp"

#include <array>
#include <functional>
#include <string>
#include <glm/glm.hpp>

class Transform;
class PickupComponent;
class PlacementComponent;
class HierarchyObject;

class PlayerController : public ComponentBase,
                         public gbe::EventHandler,
                         public gbe::ITrigger<UpdateTrigger>,
                         public gbe::ITrigger<OnGUI_Release> {
public:
    PlayerController(Transform* transform = nullptr, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~PlayerController() noexcept override;

    PlayerController(const PlayerController&) = delete;
    PlayerController& operator=(const PlayerController&) = delete;

    PlayerController(PlayerController&&) = default;
    PlayerController& operator=(PlayerController&&) = default;

    void OnUpdate(float deltaTime) override;
    void OnGUI_ReleaseEvent(float deltaTime) override;

    void SetHandTransform(gbe::ObjectRef<Transform> handTransform) { m_handTransform = handTransform; }
    gbe::ObjectRef<Transform> GetHandTransform() const { return m_handTransform; }

private:
    struct InventorySlot {
        bool occupied = false;
        std::string itemId;
    };

    struct PickupCandidate {
        PickupComponent* component = nullptr;
        Transform* transform = nullptr;
        float distance = 0.0f;
    };

    struct PlacementCandidate {
        PlacementComponent* component = nullptr;
        float distance = 0.0f;
    };

    static void ForEachObject(const std::function<void(HierarchyObject*)>& fn);

    PickupCandidate FindBestPickupCandidate(const glm::vec3& cameraPos, const glm::vec3& cameraForward) const;
    PlacementCandidate FindBestPlacementCandidate(const glm::vec3& cameraPos, const glm::vec3& cameraForward) const;

    bool TryPickupIntoSelectedSlot(const PickupCandidate& candidate);
    bool TryPlaceOrTakeWithSelectedSlot(const PlacementCandidate& candidate);

    bool EnsureHandVisual();
    void DestroyHandVisual();
    void UpdateHandVisual(bool shouldShow);
 
    void DrawHotbar();

    Transform* m_transform = nullptr;

    std::array<InventorySlot, 4> m_inventory;
    int m_selectedSlot = 0;
    GBE_SERIALIZE_FIELD_W_NAME(m_selectedSlot, "Selected Hotbar Slot");

    gbe::ObjectRef<Transform> m_handTransform = nullptr;
    GBE_SERIALIZE_FIELD_W_NAME(m_handTransform, "Hand Transform");

    bool m_primaryPressedThisFrame = false;
    gbe::IInstanceManager<HierarchyObject>::Ref m_handVisualObject = nullptr;

    void GBE_Init() override;
    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PlayerController, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(PlayerController, ComponentBase);
