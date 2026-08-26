#include "Transform.hpp"
#include "Physics/PhysicsBase.hpp"
#include "../HierarchyObject.hpp"


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
            physics->Teleport(m_position, m_rotation);
        }
    }
}