#pragma once

#include "Components/ComponentBase.hpp"
#include "Components/Transform.hpp"
#include "../Physics/RigidBody.hpp"
#include "../../HierarchyObject.hpp"
#include "Example/PlayerInput.hpp"
#include "../../../Physics/PhysicsEngine.hpp"
#include "../../../ObjectSystems/Event/Types/FixedUpdateTrigger.hpp"
#include <cmath>

class PlayerMovementComponent : public ComponentBase, public gbe::ITrigger<FixedUpdateTrigger>
{
public:
    PlayerMovementComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner = nullptr)
        : ComponentBase("PlayerMovementComponent", owner),
          m_moveSpeed(5.0f) {}

    ~PlayerMovementComponent() override = default;

    PlayerMovementComponent(const PlayerMovementComponent &) = delete;
    PlayerMovementComponent &operator=(const PlayerMovementComponent &) = delete;

    PlayerMovementComponent(PlayerMovementComponent &&) = default;
    PlayerMovementComponent &operator=(PlayerMovementComponent &&) = default;

    void OnFixedUpdate(float) override
    {
        Transform *transform = m_targetTransform.Get();
        RigidBody *rigidBody = m_targetBody.Get();
        if (!rigidBody && m_owner.GetPtr())
        {
            rigidBody = m_owner.GetPtr()->GetComponent<RigidBody>();
        }

        bool jumpHeld = static_cast<bool>(m_input.isJumping);
        bool jumpPressed = jumpHeld && !m_jumpHeld;
        m_jumpHeld = jumpHeld;

        if (!transform || !rigidBody || !rigidBody->GetBody())
        {
            return;
        }

        glm::vec2 moveDir = m_input.GetMovementVector();
        constexpr float epsilon = 0.0001f;
        float moveLengthSquared = glm::dot(moveDir, moveDir);

        if (moveLengthSquared > epsilon * epsilon)
        {
            glm::quat rotation = transform->GetRotation();
            glm::vec3 front = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
            glm::vec3 right = rotation * glm::vec3(1.0f, 0.0f, 0.0f);

            right.y = 0.0f;
            front.y = 0.0f;

            float frontLengthSquared = glm::dot(front, front);
            if (frontLengthSquared > epsilon * epsilon)
            {
                front *= 1.0f / std::sqrt(frontLengthSquared);

                glm::vec3 planarRight(front.z, 0.0f, -front.x);
                if (glm::dot(planarRight, right) < 0.0f)
                {
                    planarRight = -planarRight;
                }
                right = planarRight;
            }
            else
            {
                float rightLengthSquared = glm::dot(right, right);
                if (rightLengthSquared > epsilon * epsilon)
                {
                    right *= 1.0f / std::sqrt(rightLengthSquared);
                    front = glm::vec3(-right.z, 0.0f, right.x);
                }
                else
                {
                    front = glm::vec3(0.0f, 0.0f, 1.0f);
                    right = glm::vec3(1.0f, 0.0f, 0.0f);
                }
            }

            float moveLength = std::sqrt(moveLengthSquared);

            glm::vec3 movement = (right * moveDir.x + front * moveDir.y) / moveLength;
            glm::vec3 velocity = rigidBody->GetVelocity();
            velocity.x = movement.x * m_moveSpeed;
            velocity.z = movement.z * m_moveSpeed;
            rigidBody->SetVelocity(velocity);
        }
        else
        {
            glm::vec3 velocity = rigidBody->GetVelocity();
            velocity.x = 0.0f;
            velocity.z = 0.0f;
            rigidBody->SetVelocity(velocity);
        }

        if (jumpPressed && IsGrounded(*rigidBody))
        {
            rigidBody->ApplyImpulse(glm::vec3(0.0f, m_jumpImpulse, 0.0f));
        }
    }

    // Reference management
    void SetTargetTransform(gbe::ObjectRef<Transform> target) { m_targetTransform = target; }
    gbe::ObjectRef<Transform> GetTargetTransform() const { return m_targetTransform; }
    void SetTargetBody(gbe::ObjectRef<RigidBody> target) { m_targetBody = target; }
    gbe::ObjectRef<RigidBody> GetTargetBody() const { return m_targetBody; }

    // Speed controls
    void SetMoveSpeed(float speed) { m_moveSpeed = speed; }
    float GetMoveSpeed() const { return m_moveSpeed; }

private:
    bool IsGrounded(const RigidBody& rigidBody) const
    {
        std::shared_ptr<IPhysicsBody> hitBody;
        glm::vec3 hitPoint;
        const glm::vec3 origin = rigidBody.GetBody()->GetPosition() + glm::vec3(0.0f, m_groundCheckOffset, 0.0f);
        return PhysicsEngine::GetInstance().Get().Raycast(
            origin, glm::vec3(0.0f, -1.0f, 0.0f), m_groundCheckDistance,
            hitBody, hitPoint, rigidBody.GetBody());
    }

    PlayerInput m_input;

    gbe::ObjectRef<Transform> m_targetTransform = nullptr;
    GBE_SERIALIZE_FIELD_W_NAME(m_targetTransform, "Target Transform");

    gbe::ObjectRef<RigidBody> m_targetBody = nullptr;
    GBE_SERIALIZE_FIELD_W_NAME(m_targetBody, "Target Rigid Body");

    float m_moveSpeed = 5.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_moveSpeed, "Move Speed");

    float m_jumpImpulse = 5.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_jumpImpulse, "Jump Impulse");

    float m_groundCheckOffset = 0.1f;
    GBE_SERIALIZE_FIELD_W_NAME(m_groundCheckOffset, "Ground Check Offset");

    float m_groundCheckDistance = 0.5f;
    GBE_SERIALIZE_FIELD_W_NAME(m_groundCheckDistance, "Ground Check Distance");

    bool m_jumpHeld = false;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PlayerMovementComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(PlayerMovementComponent, ComponentBase);