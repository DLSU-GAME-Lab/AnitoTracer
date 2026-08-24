#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "AssignableEvent/AssignableEvent.hpp"

#include <glm/glm.hpp>

class Transform;
class KeyComponent;

class LockComponent : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    LockComponent(Transform* transform = nullptr, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~LockComponent() override;

    LockComponent(const LockComponent&) = delete;
    LockComponent& operator=(const LockComponent&) = delete;

    LockComponent(LockComponent&&) = default;
    LockComponent& operator=(LockComponent&&) = default;

    virtual void OnUpdate(float deltaTime) override;

    // Target Key Configuration
    void SetTargetKey(gbe::ObjectRef<KeyComponent> key) { m_targetKey = key; }
    gbe::ObjectRef<KeyComponent> GetTargetKey() const { return m_targetKey; }

    // Proximity Distance Configuration
    void SetUnlockDistance(float dist) { m_unlockDistance = dist; }
    float GetUnlockDistance() const { return m_unlockDistance; }

    bool IsUnlocked() const { return m_isUnlocked; }

private:
    void OnUnlock();

    Transform* m_transform = nullptr;
    bool m_isUnlocked = false;

    gbe::ObjectRef<KeyComponent> m_targetKey = nullptr;
    GBE_SERIALIZE_FIELD_W_NAME(m_targetKey, "Target Key");

    float m_unlockDistance = 2.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_unlockDistance, "Unlock Distance");

    gbe::UnityEvent m_onProximityCallback;
    GBE_SERIALIZE_FIELD_W_NAME(m_onProximityCallback, "On Proximity Callback");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(LockComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(LockComponent, ComponentBase);