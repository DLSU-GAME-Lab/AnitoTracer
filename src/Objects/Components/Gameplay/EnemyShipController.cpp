#include "EnemyShipController.hpp"

#include "Health.hpp"
#include "PlayerController.hpp"
#include "PlayerMovementComponent.hpp"
#include "ShipWeapon.hpp"
#include "HierarchyManager.hpp"
#include "HierarchyObject.hpp"
#include "Components/Transform.hpp"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

EnemyShipController::EnemyShipController(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("EnemyShipController", owner) {}

void EnemyShipController::OnUpdate(float /*deltaTime*/) {
    HierarchyObject* owner = GetOwner().GetPtr();
    Transform* transform = owner ? owner->GetTransform() : nullptr;
    if (!transform) {
        return;
    }

    // ITrigger<UpdateTrigger> has no start callback, so hook on the first tick.
    if (!m_healthHooked) {
        m_healthHooked = true;
        if (Health* health = owner->GetComponent<Health>()) {
            gbe::UnityEvent& onDepleted = health->GetOnDepleted();
            if (!onDepleted.targetObject.Get() || onDepleted.targetMethodName.empty()) {
                onDepleted.targetObject.Set(this);
                onDepleted.targetMethodName = "OnDestroy";
            }
        }
    }

    PlayerMovementComponent* movement = owner->GetComponent<PlayerMovementComponent>();
    if (!movement) {
        return;
    }

    PlayerController* player = gbe::IInstanceManager<PlayerController>::getOldest();
    HierarchyObject* playerObject = player ? player->GetOwner().GetPtr() : nullptr;
    Transform* playerTransform = playerObject ? playerObject->GetTransform() : nullptr;
    const Health* playerHealth = playerObject ? playerObject->GetComponent<Health>() : nullptr;
    if (!playerTransform || (playerHealth && playerHealth->IsDepleted())) {
        movement->SetInput({});
        return;
    }

    const glm::vec3 toTarget = playerTransform->GetPosition() - transform->GetPosition();
    const float distance = glm::length(toTarget);
    if (distance < 0.0001f) {
        return;
    }

    // Direction to the player in the ship's local space (+Z forward, +X right, +Y up).
    const glm::vec3 local = glm::inverse(transform->GetRotation()) * (toTarget / distance);
    const float yawError = std::atan2(local.x, local.z);
    const float pitchError = std::atan2(local.y, std::sqrt(local.x * local.x + local.z * local.z));
    const float aimError = std::acos(std::clamp(local.z, -1.0f, 1.0f));

    PlayerMovementComponent::ShipInput input;
    input.yaw = std::clamp(yawError * m_turnGain, -1.0f, 1.0f);
    input.pitch = std::clamp(pitchError * m_turnGain, -1.0f, 1.0f);
    if (distance > m_keepDistance && local.z > 0.0f) {
        input.thrust = 1.0f;
    }
    movement->SetInput(input);

    if (distance <= m_fireRange && aimError <= glm::radians(m_fireConeDegrees)) {
        for (const auto& component : owner->GetComponents()) {
            if (ShipWeapon* weapon = dynamic_cast<ShipWeapon*>(component.get())) {
                weapon->Shoot();
            }
        }
    }
}

void EnemyShipController::OnDestroy() {
    if (HierarchyObject* owner = GetOwner().GetPtr()) {
        HierarchyManager::GetInstance().QueueObjectDeletion(owner->getRef());
    }
}
