#pragma once

#include "Components/ComponentBase.hpp"
#include "Components/Transform.hpp"
#include "Example/PlayerInput.hpp"

class PlayerMovementComponent : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    PlayerMovementComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner = nullptr)
        : ComponentBase("PlayerMovementComponent", owner),
        m_moveSpeed(5.0f) {}

    ~PlayerMovementComponent() override = default;

    PlayerMovementComponent(const PlayerMovementComponent&) = delete;
    PlayerMovementComponent& operator=(const PlayerMovementComponent&) = delete;

    PlayerMovementComponent(PlayerMovementComponent&&) = default;
    PlayerMovementComponent& operator=(PlayerMovementComponent&&) = default;

    void OnUpdate(float deltaTime) override {
        Transform* transform = m_targetTransform.Get();
        if (!transform) {
            return;
        }

        glm::vec2 moveDir = m_input.GetMovementVector();
        float lengthSq = glm::length(moveDir);

        // Safe length check to prevent division by zero
        if (lengthSq > 0.0001f) {
            glm::vec3 currentPos = transform->GetPosition();

            // If GetMovementVector doesn't normalize internally:
            glm::vec2 normDir = moveDir / std::sqrt(lengthSq);

            currentPos.x += normDir.x * m_moveSpeed * deltaTime;
            currentPos.z += normDir.y * m_moveSpeed * deltaTime;

            transform->SetPosition(currentPos);
        }
    }

    // Reference management
    void SetTargetTransform(gbe::ObjectRef<Transform> target) { m_targetTransform = target; }
    gbe::ObjectRef<Transform> GetTargetTransform() const { return m_targetTransform; }

    // Speed controls
    void SetMoveSpeed(float speed) { m_moveSpeed = speed; }
    float GetMoveSpeed() const { return m_moveSpeed; }

private:
    PlayerInput m_input;

    gbe::ObjectRef<Transform> m_targetTransform = nullptr;
    GBE_SERIALIZE_FIELD_W_NAME(m_targetTransform, "Target Transform");

    float m_moveSpeed = 5.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_moveSpeed, "Move Speed");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PlayerMovementComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(PlayerMovementComponent, ComponentBase);