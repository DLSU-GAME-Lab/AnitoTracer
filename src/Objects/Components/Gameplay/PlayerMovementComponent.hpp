#pragma once

#include "Components/ComponentBase.hpp"
#include "Components/Transform.hpp"
#include "Example/PlayerInput.hpp"

class PlayerMovementComponent : public ComponentBase, public gbe::ITrigger<UpdateTrigger>
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

    void OnUpdate(float deltaTime) override
    {
        Transform *transform = m_targetTransform.Get();
        if (!transform)
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

            glm::vec3 currentPos = transform->GetPosition();
            float moveLength = std::sqrt(moveLengthSquared);

            glm::vec3 movement = (right * moveDir.x + front * moveDir.y) / moveLength;
            currentPos += movement * m_moveSpeed * deltaTime;
            currentPos.y = transform->GetPosition().y;

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