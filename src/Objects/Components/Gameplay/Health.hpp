#pragma once

#include "Components/ComponentBase.hpp"
#include "AssignableEvent/AssignableEvent.hpp"

class Health : public ComponentBase {
public:
    Health(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~Health() override = default;

    Health(const Health&) = delete;
    Health& operator=(const Health&) = delete;

    Health(Health&&) = default;
    Health& operator=(Health&&) = default;

    // Returns true if this hit depleted the health.
    bool TakeDamage(float amount);
    void Heal(float amount);

    float GetCurrentHealth();
    float GetMaxHealth() const { return m_maxHealth; }
    bool IsDepleted() const { return m_depleted; }
    gbe::UnityEvent& GetOnDepleted() { return m_onDepleted; }

    // Radius used by projectiles for hit tests, scaled by the owner's largest scale axis.
    float GetHitRadius() const { return m_hitRadius; }

private:
    float m_maxHealth = 100.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_maxHealth, "Max Health");

    float m_hitRadius = 1.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_hitRadius, "Hit Radius");

    gbe::UnityEvent m_onDepleted;
    GBE_SERIALIZE_FIELD_W_NAME(m_onDepleted, "On Depleted");

    float m_currentHealth = -1.0f; // lazily initialised to max health
    bool m_depleted = false;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(Health, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(Health, ComponentBase);
