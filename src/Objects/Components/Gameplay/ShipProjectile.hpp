#pragma once

#include "Components/ComponentBase.hpp"
#include "Types/UpdateTrigger.hpp"
#include "Types/FixedUpdateTrigger.hpp"
#include "../../../Physics/IPhysicsEngine.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <mutex>
#include <vector>

class HierarchyObject;

// Driven by a RigidBody + sphere Collider (added by ShipWeapon); damages the first
// Health-bearing object the physics system reports a collision with.
class ShipProjectile
    : public ComponentBase
    , public gbe::ITrigger<UpdateTrigger>
    , public gbe::ITrigger<FixedUpdateTrigger>
    , public gbe::ITrigger<CollisionEnterTrigger> {
public:
    ShipProjectile(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~ShipProjectile() override = default;

    ShipProjectile(const ShipProjectile&) = delete;
    ShipProjectile& operator=(const ShipProjectile&) = delete;

    ShipProjectile(ShipProjectile&&) = default;
    ShipProjectile& operator=(ShipProjectile&&) = default;

    void OnUpdate(float deltaTime) override;
    void OnFixedUpdate(float deltaTime) override;
    void OnTrigger(const CollisionEnterTrigger& trigger) override;

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

    float m_age = 0.0f;
    gbe::IInstanceManager<HierarchyObject>::Ref m_instigator = {};

    // Collision callbacks may come from physics worker threads, so hits are queued
    // here and resolved on the main thread in OnUpdate.
    std::unique_ptr<std::mutex> m_hitMutex = std::make_unique<std::mutex>();
    std::vector<gbe::IInstanceManager<HierarchyObject>::Ref> m_pendingHits;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(ShipProjectile, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(ShipProjectile, ComponentBase);
