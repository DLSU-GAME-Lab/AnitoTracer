#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "AssignableEvent/MethodRegistry.hpp"

#include <glm/glm.hpp>

class ShipWeapon : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    ShipWeapon(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~ShipWeapon() override = default;

    ShipWeapon(const ShipWeapon&) = delete;
    ShipWeapon& operator=(const ShipWeapon&) = delete;

    ShipWeapon(ShipWeapon&&) = default;
    ShipWeapon& operator=(ShipWeapon&&) = default;

    void OnUpdate(float deltaTime) override;

    // Exposed as a UnityFunction; fires one projectile if the cooldown allows.
    void Shoot();

private:
    float m_damage = 10.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_damage, "Damage");

    float m_projectileSpeed = 60.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_projectileSpeed, "Projectile Speed");

    float m_projectileLifetime = 5.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_projectileLifetime, "Projectile Lifetime (s)");

    float m_projectileScale = 0.3f;
    GBE_SERIALIZE_FIELD_W_NAME(m_projectileScale, "Projectile Scale");

    float m_fireInterval = 0.2f;
    GBE_SERIALIZE_FIELD_W_NAME(m_fireInterval, "Fire Interval (s)");

    glm::vec3 m_muzzleOffset = glm::vec3(0.0f, 0.0f, 1.5f);
    GBE_SERIALIZE_FIELD_W_NAME(m_muzzleOffset, "Muzzle Offset");

    float m_cooldown = 0.0f;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(ShipWeapon, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(ShipWeapon, ComponentBase);

GBE_REGISTER_METHOD(ShipWeapon, Shoot);
