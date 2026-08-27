#include "Transform.hpp"
#include "Physics/PhysicsBase.hpp"
#include "../HierarchyObject.hpp"

#include <cmath>

namespace {
const Transform* GetParentTransform(const Transform& transform) {
    HierarchyObject* owner = transform.GetOwner().GetPtr();
    if (!owner) {
        return nullptr;
    }

    HierarchyObject::Ref parent = owner->GetParent();
    if (!parent) {
        return nullptr;
    }

    return parent.GetPtr()->GetTransform();
}

glm::vec3 DivideSafe(const glm::vec3& value, const glm::vec3& divisor) {
    constexpr float Epsilon = 1e-6f;
    return glm::vec3(
        std::abs(divisor.x) > Epsilon ? value.x / divisor.x : 0.0f,
        std::abs(divisor.y) > Epsilon ? value.y / divisor.y : 0.0f,
        std::abs(divisor.z) > Epsilon ? value.z / divisor.z : 0.0f
    );
}
}

glm::vec3 Transform::GetPosition() const {
    if (const Transform* parentTransform = GetParentTransform(*this)) {
        const glm::vec3 parentPos = parentTransform->GetPosition();
        const glm::quat parentRot = parentTransform->GetRotation();
        const glm::vec3 parentScale = parentTransform->GetScale();
        return parentPos + (parentRot * (parentScale * m_position));
    }

    return m_position;
}

glm::quat Transform::GetRotation() const {
    if (const Transform* parentTransform = GetParentTransform(*this)) {
        return parentTransform->GetRotation() * m_rotation;
    }

    return m_rotation;
}

glm::vec3 Transform::GetScale() const {
    if (const Transform* parentTransform = GetParentTransform(*this)) {
        return parentTransform->GetScale() * m_scale;
    }

    return m_scale;
}

void Transform::SetWorldPosition(const glm::vec3& worldPosition) {
    if (const Transform* parentTransform = GetParentTransform(*this)) {
        const glm::vec3 parentPos = parentTransform->GetPosition();
        const glm::quat parentRot = parentTransform->GetRotation();
        const glm::vec3 parentScale = parentTransform->GetScale();

        const glm::vec3 parentSpaceOffset = glm::inverse(parentRot) * (worldPosition - parentPos);
        m_position = DivideSafe(parentSpaceOffset, parentScale);
        return;
    }

    m_position = worldPosition;
}


// Converts the current quaternion rotation to Euler angles in degrees for UI display.
glm::vec3 Transform::GetEulerAnglesDegrees() const {
    return m_eulerAnglesDegrees;
}

// Update both the cached Euler angles (for the UI) and the quaternion (for math).
void Transform::SetEulerAnglesDegrees(const glm::vec3& eulerDegrees) {
    // Store the continuous values so ImGui dragging doesn't snap
    m_eulerAnglesDegrees = eulerDegrees;

    // glm::quat constructor from euler angles expects radians.
    m_rotation = glm::quat(glm::radians(eulerDegrees));
}

glm::mat4 Transform::GetLocalMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, m_position);
    model *= glm::mat4_cast(m_rotation);
    model = glm::scale(model, m_scale);

    return model;
}

void Transform::SyncPhysics() {
    if (HierarchyObject* owner = m_owner.GetPtr()) {
        if (PhysicsBase* physics = owner->GetComponent<PhysicsBase>()) {
            physics->Teleport(GetPosition(), GetRotation());
        }
    }
}