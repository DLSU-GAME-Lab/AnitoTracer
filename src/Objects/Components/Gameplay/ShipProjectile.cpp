#include "ShipProjectile.hpp"

#include "Health.hpp"
#include "HierarchyManager.hpp"
#include "HierarchyObject.hpp"
#include "Components/Transform.hpp"

#include <algorithm>
#include <functional>

namespace {
    void ForEachObject(HierarchyObject* object, const std::function<bool(HierarchyObject*)>& fn) {
        if (!object || !fn(object)) {
            return;
        }
        for (const auto& child : object->GetChildren()) {
            ForEachObject(child.get(), fn);
        }
    }

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

    // Distance from point to segment [a, b].
    float DistanceToSegment(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b) {
        const glm::vec3 ab = b - a;
        const float lengthSq = glm::dot(ab, ab);
        const float t = lengthSq > 0.0f ? std::clamp(glm::dot(p - a, ab) / lengthSq, 0.0f, 1.0f) : 0.0f;
        return glm::distance(p, a + ab * t);
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

void ShipProjectile::OnUpdate(float deltaTime) {
    HierarchyObject* owner = GetOwner().GetPtr();
    Transform* transform = owner ? owner->GetTransform() : nullptr;
    if (!transform) {
        return;
    }

    m_age += deltaTime;
    if (m_age >= m_lifetime) {
        HierarchyManager::GetInstance().QueueObjectDeletion(owner->getRef());
        return;
    }

    const glm::vec3 start = transform->GetPosition();
    const glm::vec3 end = start + m_direction * (m_speed * deltaTime);
    transform->SetWorldPosition(end);

    HierarchyObject* instigatorRoot = GetRoot(m_instigator.GetPtr());

    Health* closestHealth = nullptr;
    float closestDistance = 0.0f;

    for (const auto& root : HierarchyManager::GetInstance().GetRootObjects()) {
        if (!root || root.get() == instigatorRoot) {
            continue;
        }

        ForEachObject(root.get(), [&](HierarchyObject* candidate) {
            if (candidate == owner) {
                return true;
            }

            Health* health = candidate->GetComponent<Health>();
            Transform* targetTransform = candidate->GetTransform();
            if (!health || !targetTransform || health->IsDepleted()) {
                return true;
            }

            const glm::vec3 scale = glm::abs(targetTransform->GetScale());
            const float hitRadius = health->GetHitRadius() * std::max({ scale.x, scale.y, scale.z }) + m_radius;
            const float distance = DistanceToSegment(targetTransform->GetPosition(), start, end);
            if (distance <= hitRadius && (!closestHealth || distance < closestDistance)) {
                closestHealth = health;
                closestDistance = distance;
            }
            return true;
        });
    }

    if (closestHealth) {
        closestHealth->TakeDamage(m_damage);
        HierarchyManager::GetInstance().QueueObjectDeletion(owner->getRef());
    }
}
