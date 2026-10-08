#pragma once

#include "Components/ComponentBase.hpp"
#include "Components/Transform.hpp"
#include "../../HierarchyObject.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>

class ObjectRotatorComponent : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    ObjectRotatorComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner = nullptr)
        : ComponentBase("ObjectRotatorComponent", owner),
          m_rotationAxis(0.0f, 1.0f, 0.0f),
          m_rotationSpeed(45.0f) {}

    ~ObjectRotatorComponent() override = default;

    ObjectRotatorComponent(const ObjectRotatorComponent&) = delete;
    ObjectRotatorComponent& operator=(const ObjectRotatorComponent&) = delete;

    ObjectRotatorComponent(ObjectRotatorComponent&&) = default;
    ObjectRotatorComponent& operator=(ObjectRotatorComponent&&) = default;

    void OnUpdate(float deltaTime) override {
        Transform* transform = m_targetTransform.Get();
        
        // Fallback to owner's Transform component if target transform isn't manually assigned
        if (!transform && m_owner.GetPtr()) {
            transform = m_owner.GetPtr()->GetComponent<Transform>();
        }

        if (!transform) {
            return;
        }

        // Normalize axis to prevent scaling artifacts
        float axisLengthSq = glm::dot(m_rotationAxis, m_rotationAxis);
        constexpr float epsilon = 0.0001f;
        if (axisLengthSq < epsilon * epsilon) {
            return;
        }

        glm::vec3 normalizedAxis = m_rotationAxis / std::sqrt(axisLengthSq);

        // Calculate delta rotation quaternion around specified axis
        float deltaAngleRadians = glm::radians(m_rotationSpeed * deltaTime);
        glm::quat deltaRotation = glm::angleAxis(deltaAngleRadians, normalizedAxis);

        // Apply relative rotation
        glm::quat currentRotation = transform->GetRotation();
        transform->SetRotation(glm::normalize(deltaRotation * currentRotation));
    }

    // Reference Management
    void SetTargetTransform(gbe::ObjectRef<Transform> target) { m_targetTransform = target; }
    gbe::ObjectRef<Transform> GetTargetTransform() const { return m_targetTransform; }

    // Rotation Control Accessors
    void SetRotationAxis(const glm::vec3& axis) { m_rotationAxis = axis; }
    glm::vec3 GetRotationAxis() const { return m_rotationAxis; }

    void SetRotationSpeed(float speed) { m_rotationSpeed = speed; }
    float GetRotationSpeed() const { return m_rotationSpeed; }

private:
    gbe::ObjectRef<Transform> m_targetTransform = nullptr;
    GBE_SERIALIZE_FIELD_W_NAME(m_targetTransform, "Target Transform");

    glm::vec3 m_rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    GBE_SERIALIZE_FIELD_W_NAME(m_rotationAxis, "Rotation Axis");

    float m_rotationSpeed = 45.0f; // Degrees per second
    GBE_SERIALIZE_FIELD_W_NAME(m_rotationSpeed, "Rotation Speed");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(ObjectRotatorComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(ObjectRotatorComponent, ComponentBase);