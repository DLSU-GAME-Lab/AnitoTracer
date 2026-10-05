#include "Health.hpp"

#include "PropertyDrawers/event_drawer.hpp"

#include <algorithm>

Health::Health(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("Health", owner) {}

float Health::GetCurrentHealth() {
    if (m_currentHealth < 0.0f) {
        m_currentHealth = m_maxHealth;
    }
    return m_currentHealth;
}

bool Health::TakeDamage(float amount) {
    if (m_depleted || amount <= 0.0f) {
        return false;
    }

    m_currentHealth = std::max(0.0f, GetCurrentHealth() - amount);
    if (m_currentHealth > 0.0f) {
        return false;
    }

    m_depleted = true;
    m_onDepleted.Invoke();
    return true;
}

void Health::Heal(float amount) {
    if (m_depleted || amount <= 0.0f) {
        return;
    }
    m_currentHealth = std::min(m_maxHealth, GetCurrentHealth() + amount);
}
