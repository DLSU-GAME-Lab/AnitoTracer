#include "PlayerController.hpp"

#include "PlayerMovementComponent.hpp"
#include "ShipWeapon.hpp"
#include "HierarchyObject.hpp"

PlayerController::PlayerController(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("PlayerController", owner) {}

void PlayerController::OnUpdate(float /*deltaTime*/) {
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) {
        return;
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
