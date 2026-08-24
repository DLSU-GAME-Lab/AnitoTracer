#include "LockComponent.hpp"
#include "KeyComponent.hpp"
#include "PickupComponent.hpp"
#include "HierarchyObject.hpp"
#include "Components/Transform.hpp"

#include "PropertyDrawers/event_drawer.hpp"
#include <iostream>

LockComponent::LockComponent(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("LockComponent", owner), m_transform(transform) {}

LockComponent::~LockComponent() = default;

void LockComponent::OnUpdate(float /*deltaTime*/) {
    // 1. dereference target key component via ObjectRef
    KeyComponent* key = m_targetKey.Get();
    if (!key) return;

    // 2. Resolve lock object transform
    Transform* lockTransform = m_transform;
    if (!lockTransform && GetOwner().GetPtr()) {
        lockTransform = GetOwner().GetPtr()->GetTransform();
    }
    if (!lockTransform) return;

    // 3. Resolve key owner object and transform
    HierarchyObject* keyOwner = key->GetOwner().GetPtr();
    if (!keyOwner) return;

    Transform* keyTransform = keyOwner->GetTransform();
    if (!keyTransform) return;

    // 4. Measure distance and evaluate pickup state
    float distance = glm::distance(lockTransform->GetPosition(), keyTransform->GetPosition());
    bool validKeyInRange = false;

    if (distance <= m_unlockDistance) {
        PickupComponent* pickupComp = keyOwner->GetComponent<PickupComponent>();

        // Valid if no PickupComponent exists OR if it exists and is NOT currently held
        if (!pickupComp || !pickupComp->IsPickedUp()) {
            validKeyInRange = true;
        }
    }

    // 5. Trigger event on enter/exit threshold state
    if (validKeyInRange && !m_isUnlocked) {
        m_isUnlocked = true;
        OnUnlock();
    }
    else if (!validKeyInRange && m_isUnlocked) {
        m_isUnlocked = false;
    }
}

void LockComponent::OnUnlock() {
    std::cout << "LockComponent: Key placed in proximity!" << std::endl;
    m_onProximityCallback.Invoke();
}