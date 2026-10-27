#include "PlayerController.hpp"

#include "PlayerMovementComponent.hpp"
#include "ShipWeapon.hpp"
#include "HierarchyObject.hpp"
#include "Components/Particles/ParticleSystemComponent.cpp"

PlayerController::PlayerController(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("PlayerController", owner) {}

namespace {
    void CollectParticles(HierarchyObject* object, std::vector<ParticleSystemComponent*>& out) {
        if (!object) return;

        for (const auto& component : object->GetComponents()) {
            if (auto* particles = dynamic_cast<ParticleSystemComponent*>(component.get())) {
                out.push_back(particles);
            }
        }

        for (const auto& child : object->GetChildren()) {
            CollectParticles(child.get(), out);
        }
    }
}

void PlayerController::SetThrustParticles(bool active) {
    for (ParticleSystemComponent* particles : m_thrustParticles) {
        if (active) {
            particles->Play();
        } else {
            particles->Stop();
        }
    }
    m_particlesActive = active;
}

void PlayerController::OnStart() {
    m_thrustParticles.clear();
    CollectParticles(GetOwner().GetPtr(), m_thrustParticles);
    SetThrustParticles(m_input.isThrustingForward);
}

void PlayerController::OnUpdate(float /*deltaTime*/) {
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) {
        return;
    }

    if (static_cast<bool>(m_input.isThrustingForward) != m_particlesActive) {
        SetThrustParticles(m_input.isThrustingForward);
    }

    if (PlayerMovementComponent* movement = owner->GetComponent<PlayerMovementComponent>()) {
        PlayerMovementComponent::ShipInput input;
        input.thrust = static_cast<float>(m_input.isThrustingForward) - static_cast<float>(m_input.isThrustingBackward);
        input.roll = static_cast<float>(m_input.isRollingRight) - static_cast<float>(m_input.isRollingLeft);
        input.pitch = static_cast<float>(m_input.isPitchingUp) - static_cast<float>(m_input.isPitchingDown);
        input.yaw = static_cast<float>(m_input.isYawingRight) - static_cast<float>(m_input.isYawingLeft);
        movement->SetInput(input);
    }

    if (m_input.isPrimary) {
        for (const auto& component : owner->GetComponents()) {
            if (ShipWeapon* weapon = dynamic_cast<ShipWeapon*>(component.get())) {
                weapon->Shoot();
            }
        }
    }
}
