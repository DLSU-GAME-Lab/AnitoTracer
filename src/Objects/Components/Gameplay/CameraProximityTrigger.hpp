#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "AssignableEvent/AssignableEvent.hpp"

#include <glm/glm.hpp>
#include <functional>

class Transform;
class CameraComponent;

class CameraProximityTrigger : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    CameraProximityTrigger(Transform* transform = nullptr, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~CameraProximityTrigger() override;

    CameraProximityTrigger(const CameraProximityTrigger&) = delete;
    CameraProximityTrigger& operator=(const CameraProximityTrigger&) = delete;

    CameraProximityTrigger(CameraProximityTrigger&&) = default;
    CameraProximityTrigger& operator=(CameraProximityTrigger&&) = default;

    virtual void OnUpdate(float deltaTime) override;

    // Configuration parameters
    void SetTriggerDistance(float dist) { m_triggerDistance = dist; }
    float GetTriggerDistance() const { return m_triggerDistance; }

    bool IsCameraInRange() const { return m_isInRange; }
private:
    void OnProximityEnter();
    void DrawConfirmationPrompt();

    Transform* m_transform = nullptr;
    bool m_isInRange = false;
    bool m_waitingForConfirmation = false;

    float m_triggerDistance = 3.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_triggerDistance, "Trigger Distance");

	gbe::UnityEvent m_onProximityCallback;
	GBE_SERIALIZE_FIELD_W_NAME(m_onProximityCallback, "On Proximity Callback");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(CameraProximityTrigger, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(CameraProximityTrigger, ComponentBase);