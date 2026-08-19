#pragma once

#include "Components/ComponentBase.hpp"
#include "EventHandler.hpp"
#include "Types/UpdateTrigger.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// Forward declarations to keep this header lightweight. Implementation details are
// moved to the .cpp file to avoid pulling heavy transitive includes into every
// translation unit that includes this header.
class Transform;
class CameraComponent;

class PickupComponent : public ComponentBase, public gbe::EventHandler, public gbe::ITrigger<UpdateTrigger> {
public:
    PickupComponent(Transform* transform = nullptr, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~PickupComponent() override;

    PickupComponent(const PickupComponent&) = delete;
    PickupComponent& operator=(const PickupComponent&) = delete;

    PickupComponent(PickupComponent&&) = default;
    PickupComponent& operator=(PickupComponent&&) = default;

    virtual void OnUpdate(float deltaTime) override;

    // Configurable parameters
    void SetMaxPickupDistance(float dist) { m_maxPickupDistance = dist; }
    float GetMaxPickupDistance() const { return m_maxPickupDistance; }

    void SetFacingThreshold(float dot) { m_facingThreshold = dot; }
    float GetFacingThreshold() const { return m_facingThreshold; }

    bool IsPickedUp() const { return m_isPickedUp; }

private:
    Transform* m_transform = nullptr;

    bool m_isPickedUp = false;
    bool m_primaryPressedThisFrame = false;
    float m_holdDistance = 2.0f;

    float m_maxPickupDistance = 5.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_maxPickupDistance, "Max Pickup Distance");

    // Default dot threshold ~0.707 (within a 45-degree cone from center view)
    float m_facingThreshold = 0.707f;
    GBE_SERIALIZE_FIELD_W_NAME(m_facingThreshold, "Facing Threshold (Dot)");

    virtual void GBE_Init() override;
    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PickupComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(PickupComponent, ComponentBase);
