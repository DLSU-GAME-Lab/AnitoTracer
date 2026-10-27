#include "ShipProjectile.hpp"

#include "Health.hpp"
#include "HierarchyManager.hpp"
#include "HierarchyObject.hpp"
#include "Components/Physics/RigidBody.hpp"

#include <cmath>

namespace {
    HierarchyObject* GetRoot(HierarchyObject* object) {
        while (object) {
            HierarchyObject* parent = object->GetParent().GetPtr();
            if (!parent) {
                break;
            }
            object = parent;
        }
        return object;
    }

    // The collided body's owner may be a child of the object that carries the Health.
    Health* FindHealthInAncestry(HierarchyObject* object) {
        while (object) {
            if (Health* health = object->GetComponent<Health>()) {
                return health;
            }
            object = object->GetParent().GetPtr();
        }
        return nullptr;
    }
}

ShipProjectile::ShipProjectile(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("ShipProjectile", owner) {}

void ShipProjectile::Launch(const glm::vec3& direction, float speed, float damage, float lifetime,
                            gbe::IInstanceManager<HierarchyObject>::Ref instigator) {
    const float lengthSq = glm::dot(direction, direction);
    m_direction = lengthSq > 0.0f ? direction / std::sqrt(lengthSq) : glm::vec3(0.0f, 0.0f, 1.0f);
    m_speed = speed;
    m_damage = damage;
    m_lifetime = lifetime;
    m_instigator = instigator;
}

void ShipProjectile::OnFixedUpdate(float /*deltaTime*/) {
    HierarchyObject* owner = GetOwner().GetPtr();
    RigidBody* body = owner ? owner->GetComponent<RigidBody>() : nullptr;
    if (body && body->GetBody()) {
        // Re-asserted every tick so collisions and gravity can't deflect or slow the projectile.
        body->SetVelocity(m_direction * m_speed);
    }
}

void ShipProjectile::OnTrigger(const CollisionEnterTrigger& trigger) {
    HierarchyObject* other = PhysicsBase::FindOwnerByBody(trigger.other.get());
    if (!other) {
        return;
    }

    std::lock_guard<std::mutex> lock(*m_hitMutex);
    m_pendingHits.push_back(other->getRef());
}

void ShipProjectile::OnUpdate(float deltaTime) {
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) {
        return;
    }

    std::vector<gbe::IInstanceManager<HierarchyObject>::Ref> hits;
    {
        std::lock_guard<std::mutex> lock(*m_hitMutex);
        hits.swap(m_pendingHits);
    }

    HierarchyObject* instigatorRoot = GetRoot(m_instigator.GetPtr());
    for (const auto& hitRef : hits) {
        HierarchyObject* other = hitRef.GetPtr();
        if (!other || other == owner || GetRoot(other) == instigatorRoot || other->GetComponent<ShipProjectile>()) {
            continue;
        }

        Health* health = FindHealthInAncestry(other);
        if (health && !health->IsDepleted()) {
            health->TakeDamage(m_damage);
        }
        HierarchyManager::GetInstance().QueueObjectDeletion(owner->getRef());
        return;
    }

    m_age += deltaTime;
    if (m_age >= m_lifetime) {
        HierarchyManager::GetInstance().QueueObjectDeletion(owner->getRef());
    }
}
