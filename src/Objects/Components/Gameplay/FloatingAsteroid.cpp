#include "FloatingAsteroid.hpp"

#include "HierarchyManager.hpp"
#include "HierarchyObject.hpp"
#include "Components/Physics/RigidBody.hpp"
#include "Health.hpp"
#include "../../../Physics/PhysicsEngine.hpp"

FloatingAsteroid::FloatingAsteroid(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("FloatingAsteroid", owner) {}

void FloatingAsteroid::OnFixedUpdate(float /*deltaTime*/) {
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) {
        return;
    }

    // ITrigger<FixedUpdateTrigger> has no start callback, so hook on the first tick.
    if (!m_healthHooked) {
        m_healthHooked = true;
        if (Health* health = owner->GetComponent<Health>()) {
            gbe::UnityEvent& onDepleted = health->GetOnDepleted();
            if (!onDepleted.targetObject.Get() || onDepleted.targetMethodName.empty()) {
                onDepleted.targetObject.Set(this);
                onDepleted.targetMethodName = "OnDestroy";
            }
        }
    }

    RigidBody* body = owner->GetComponent<RigidBody>();
    if (!body || !body->GetBody() || body->GetMass() <= 0.0f) {
        return;
    }

    if (!m_tumbleApplied) {
        body->SetAngularVelocity(m_tumbleSpeed);
        m_tumbleApplied = true;
    }

    const glm::vec3 gravity = PhysicsEngine::GetInstance().Get().GetGravity();
    body->ApplyForce(-gravity * body->GetMass());
}

void FloatingAsteroid::OnDestroy() {
    if (HierarchyObject* owner = GetOwner().GetPtr()) {
        HierarchyManager::GetInstance().QueueObjectDeletion(owner->getRef());
    }
}
