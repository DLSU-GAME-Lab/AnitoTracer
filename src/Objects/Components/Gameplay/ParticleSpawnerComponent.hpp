#pragma once

#include "Components/ComponentBase.hpp"
#include "EventHandler.hpp"
#include "Types/UpdateTrigger.hpp"
#include "AssignableEvent/AssignableEvent.hpp"

#include <glm/glm.hpp>
#include <vector>

class Transform;

struct Particle {
    gbe::IInstanceManager<HierarchyObject>::Ref sphereRef;
    glm::vec3 velocity{ 0.0f };
    float currentAge = 0.0f;
    float maxLifetime = 3.0f;
};

class ParticleSpawnerComponent : public ComponentBase, public gbe::EventHandler, public gbe::ITrigger<UpdateTrigger> {
public:
    ParticleSpawnerComponent(Transform* transform = nullptr, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~ParticleSpawnerComponent() override;

    ParticleSpawnerComponent(const ParticleSpawnerComponent&) = delete;
    ParticleSpawnerComponent& operator=(const ParticleSpawnerComponent&) = delete;

    ParticleSpawnerComponent(ParticleSpawnerComponent&&) = default;
    ParticleSpawnerComponent& operator=(ParticleSpawnerComponent&&) = default;

    virtual void OnUpdate(float deltaTime) override;

    // Callable method for event binding / manual execution
    void Spawn();

    // Spawner parameters
    void SetSpawnCount(int count) { m_spawnCount = count; }
    int GetSpawnCount() const { return m_spawnCount; }

    void SetUpwardForce(float force) { m_upwardForce = force; }
    float GetUpwardForce() const { return m_upwardForce; }

    void SetGravity(float gravity) { m_gravity = gravity; }
    float GetGravity() const { return m_gravity; }

    void SetParticleLifetime(float lifetime) { m_particleLifetime = lifetime; }
    float GetParticleLifetime() const { return m_particleLifetime; }

private:
    virtual void GBE_Init() override;

    std::vector<Particle> m_activeParticles;

    int m_spawnCount = 10;
    GBE_SERIALIZE_FIELD_W_NAME(m_spawnCount, "Spawn Count");

    float m_upwardForce = 8.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_upwardForce, "Upward Force");

    float m_spread = 2.5f;
    GBE_SERIALIZE_FIELD_W_NAME(m_spread, "Velocity Spread");

    float m_gravity = 9.81f;
    GBE_SERIALIZE_FIELD_W_NAME(m_gravity, "Gravity Acceleration");

    float m_particleLifetime = 3.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_particleLifetime, "Particle Lifetime (s)");

    std::string m_triggerEventKey = "OnSpawnParticles";
    GBE_SERIALIZE_FIELD_W_NAME(m_triggerEventKey, "Trigger Event Key");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(ParticleSpawnerComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(ParticleSpawnerComponent, ComponentBase);

GBE_REGISTER_METHOD(ParticleSpawnerComponent, Spawn);