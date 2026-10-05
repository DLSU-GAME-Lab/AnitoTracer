#include "ShipWeapon.hpp"

#include "ShipProjectile.hpp"
#include "HierarchyManager.hpp"
#include "HierarchyObject.hpp"
#include "ObjectFactory.hpp"
#include "Components/Transform.hpp"

#include <algorithm>
#include <memory>

ShipWeapon::ShipWeapon(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("ShipWeapon", owner) {}

void ShipWeapon::OnUpdate(float deltaTime) {
    m_cooldown = std::max(0.0f, m_cooldown - deltaTime);
}

void ShipWeapon::Shoot() {
    if (m_cooldown > 0.0f) {
        return;
    }

    HierarchyObject* owner = GetOwner().GetPtr();
    Transform* ownerTransform = owner ? owner->GetTransform() : nullptr;
    if (!ownerTransform) {
        return;
    }

    HierarchyObject::Ref projectileRef = ObjectFactory::GetInstance().CreateSpherePrimitive("Projectile");
    HierarchyObject* projectile = projectileRef.GetPtr();
    if (!projectile) {
        return;
    }

    const glm::quat rotation = ownerTransform->GetRotation();
    const glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);

    if (Transform* projectileTransform = projectile->GetTransform()) {
        projectileTransform->SetPosition(ownerTransform->GetPosition() + rotation * m_muzzleOffset);
        projectileTransform->SetScale(glm::vec3(m_projectileScale));
    }

    auto projectileComponent = std::make_unique<ShipProjectile>(projectileRef);
    projectileComponent->Launch(forward, m_projectileSpeed, m_damage, m_projectileLifetime, GetOwner());
    HierarchyManager::GetInstance().AddComponentToObject(projectileRef, std::move(projectileComponent));

    m_cooldown = m_fireInterval;
}
