#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/OnGUI_Editor.hpp"
#include "Types/OnGUI_Release.hpp"
#include "Types/UpdateTrigger.hpp"
#include "AssignableEvent/AssignableEvent.hpp"
#include "PropertyDrawers/event_drawer.hpp"

#include <string>
#include <glm/glm.hpp>

class Transform;
class HierarchyObject;
class CameraComponent;
struct ImVec2;

class PlacementComponent : public ComponentBase,
                           public gbe::ITrigger<UpdateTrigger>,
                           public gbe::ITrigger<OnGUI_Release>,
                           public gbe::ITrigger<OnGUI_Editor> {
public:
    PlacementComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~PlacementComponent() noexcept override;

    PlacementComponent(const PlacementComponent&) = delete;
    PlacementComponent& operator=(const PlacementComponent&) = delete;

    PlacementComponent(PlacementComponent&&) = default;
    PlacementComponent& operator=(PlacementComponent&&) = default;

    void OnUpdate(float deltaTime) override;
    void OnGUI_ReleaseEvent(float deltaTime) override;
    void OnGUI_EditorEvent(float deltaTime) override;

    bool CanInteract(const glm::vec3& cameraPos, const glm::vec3& cameraForward, float& outDistance) const;

    bool HasItem() const { return m_hasStoredItem; }
    const std::string& GetStoredItemId() const { return m_storedItemId; }

    bool TryPlaceItem(const std::string& itemId);
    bool TryTakeItem(std::string& outItemId);

    void SetPlacementHeight(float height) { m_placementHeight = height; }
    float GetPlacementHeight() const { return m_placementHeight; }

    void SetMaxInteractionDistance(float dist) { m_maxInteractionDistance = dist; }
    float GetMaxInteractionDistance() const { return m_maxInteractionDistance; }

    void SetFacingThreshold(float dot) { m_facingThreshold = dot; }
    float GetFacingThreshold() const { return m_facingThreshold; }

    void SetRequiredItemId(const std::string& itemId) { m_requiredItemId = itemId; }
    const std::string& GetRequiredItemId() const { return m_requiredItemId; }

    bool IsUnlocked() const { return m_isUnlocked; }

private:
    void OnUnlock();
    bool GetAnchorTransform(Transform*& outTransform) const;
    glm::vec3 GetAnchorPosition() const;
    bool EnsurePlacedVisual();
    void DestroyPlacedVisual();
    void SyncPlacedVisualTransform() const;
    void DrawPlacementOverlay(bool isEditorContext);
    bool ProjectWorldToScreen(const glm::vec3& worldPos, CameraComponent& camera, ImVec2& outScreenPos) const;

    float m_placementHeight = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_placementHeight, "Placement Height");

    bool m_hasStoredItem = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_hasStoredItem, "Has Stored Item");

    std::string m_storedItemId;
    GBE_SERIALIZE_FIELD_W_NAME(m_storedItemId, "Stored Item Id");

    float m_maxInteractionDistance = 5.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_maxInteractionDistance, "Max Interaction Distance");

    float m_facingThreshold = 0.707f;
    GBE_SERIALIZE_FIELD_W_NAME(m_facingThreshold, "Facing Threshold (Dot)");

    std::string m_requiredItemId;
    GBE_SERIALIZE_FIELD_W_NAME(m_requiredItemId, "Required Item Id");

    bool m_isUnlocked = false;

    gbe::UnityEvent m_onUnlockCallback;
    GBE_SERIALIZE_FIELD_W_NAME(m_onUnlockCallback, "On Unlock Callback");

    gbe::IInstanceManager<HierarchyObject>::Ref m_placedVisualObject = nullptr;

    void GBE_Init() override;
    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PlacementComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(PlacementComponent, ComponentBase);
