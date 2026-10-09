#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"

#include <glm/glm.hpp>

class HierarchyObject;

// Flies in a straight line and damages the first Health-bearing object it touches.
class ShipProjectile : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    ShipProjectile(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~ShipProjectile() override = default;

    ShipProjectile(const ShipProjectile&) = delete;
    ShipProjectile& operator=(const ShipProjectile&) = delete;

    ShipProjectile(ShipProjectile&&) = default;
    ShipProjectile& operator=(ShipProjectile&&) = default;

    void OnUpdate(float deltaTime) override;

    // Objects in the instigator's hierarchy root are never damaged.
    void Launch(const glm::vec3& direction, float speed, float damage, float lifetime,
                gbe::IInstanceManager<HierarchyObject>::Ref instigator);

private:
    glm::vec3 m_direction = glm::vec3(0.0f, 0.0f, 1.0f);

    float m_speed = 60.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_speed, "Speed");

    float m_damage = 10.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_damage, "Damage");

    float m_lifetime = 5.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_lifetime, "Lifetime (s)");

    float m_radius = 0.2f;
    GBE_SERIALIZE_FIELD_W_NAME(m_radius, "Radius");

    float m_age = 0.0f;
    gbe::IInstanceManager<HierarchyObject>::Ref m_instigator = {};

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(ShipProjectile, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(ShipProjectile, ComponentBase);
