#pragma once

#include "Components/ComponentBase.hpp"
#include "Components/Transform.hpp"
#include "../Physics/RigidBody.hpp"
#include "../../HierarchyObject.hpp"
#include "../../../Physics/PhysicsEngine.hpp"
#include "../../../ObjectSystems/Event/Types/FixedUpdateTrigger.hpp"

#include <algorithm>
#include <glm/glm.hpp>

// Physics for the flying ship. Input is supplied by PlayerController through SetInput().
class PlayerMovementComponent : public ComponentBase, public gbe::ITrigger<FixedUpdateTrigger>
{
public:
    struct ShipInput
    {
        float thrust = 0.0f;
        float pitch = 0.0f;
        float roll = 0.0f;
        float yaw = 0.0f;
    };

    PlayerMovementComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner = nullptr)
        : ComponentBase("PlayerMovementComponent", owner) {}

    ~PlayerMovementComponent() override = default;

    PlayerMovementComponent(const PlayerMovementComponent &) = delete;
    PlayerMovementComponent &operator=(const PlayerMovementComponent &) = delete;

    PlayerMovementComponent(PlayerMovementComponent &&) = default;
    PlayerMovementComponent &operator=(PlayerMovementComponent &&) = default;

    void OnFixedUpdate(float deltaTime) override
    {
        HierarchyObject *owner = m_owner.GetPtr();
        RigidBody *rigidBody = owner ? owner->GetComponent<RigidBody>() : nullptr;
        if (!rigidBody || !rigidBody->GetBody() || rigidBody->GetMass() <= 0.0f)
        {
            return;
        }

        const float thrust = glm::clamp(m_input.thrust, -1.0f, 1.0f);
        const float pitch = glm::clamp(m_input.pitch, -1.0f, 1.0f);
        const float roll = glm::clamp(m_input.roll, -1.0f, 1.0f);
        const float yaw = glm::clamp(m_input.yaw, -1.0f, 1.0f);
        const glm::quat rotation = rigidBody->GetBody()->GetRotation();
        const glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
        const float thrustSpeed = thrust > 0.0f ? m_forwardSpeed : m_reverseSpeed;
        const glm::vec3 targetVelocity = forward * (thrust * thrustSpeed);

        // Accelerate when thrusting, brake otherwise.
        const glm::vec3 velocity = rigidBody->GetVelocity();
        const glm::vec3 delta = targetVelocity - velocity;
        const float deltaLength = glm::length(delta);
        const float rate = glm::abs(thrust) > 0.0001f ? m_acceleration : m_deceleration;
        const float maxStep = rate * deltaTime;

        const glm::vec3 newVelocity = (deltaLength <= maxStep || deltaLength < 0.0001f)
            ? targetVelocity
            : velocity + delta * (maxStep / deltaLength);

        rigidBody->SetVelocity(newVelocity);
        const glm::vec3 localTorque(-pitch * m_pitchTorque, yaw * m_yawTorque, roll * m_rollTorque);
        const glm::vec3 controlTorque = rotation * localTorque;
        const glm::vec3 dampingTorque = -rigidBody->GetAngularVelocity() * m_angularDamping;
        rigidBody->ApplyTorque(controlTorque + dampingTorque);

        // Cancel world gravity so the ship hovers.
        rigidBody->ApplyForce(-PhysicsEngine::GetInstance().Get().GetGravity() * rigidBody->GetMass());
    }

    void SetInput(const ShipInput &input) { m_input = input; }
    const ShipInput &GetInput() const { return m_input; }

private:
    ShipInput m_input;

    float m_forwardSpeed = 20.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_forwardSpeed, "Forward Speed");

    float m_reverseSpeed = 8.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_reverseSpeed, "Reverse Speed");

    float m_pitchTorque = 2.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_pitchTorque, "Pitch Torque");

    float m_rollTorque = 2.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_rollTorque, "Roll Torque");

    float m_yawTorque = 2.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_yawTorque, "Yaw Torque");

    float m_angularDamping = 1.5f;
    GBE_SERIALIZE_FIELD_W_NAME(m_angularDamping, "Angular Damping");

    float m_acceleration = 30.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_acceleration, "Acceleration");

    float m_deceleration = 20.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_deceleration, "Deceleration");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PlayerMovementComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(PlayerMovementComponent, ComponentBase);
