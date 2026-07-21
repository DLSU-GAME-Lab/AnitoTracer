#include "Transform.hpp"


// Converts the current quaternion rotation to Euler angles in degrees for UI display.
glm::vec3 Transform::GetEulerAnglesDegrees() const {
    // glm::eulerAngles returns radians, so we convert them to degrees for the UI.
    return glm::degrees(glm::eulerAngles(m_rotation));
}

// Sets the quaternion rotation from Euler angles provided in degrees from the UI.
void Transform::SetEulerAnglesDegrees(const glm::vec3& eulerDegrees) {
    // glm::quat constructor from euler angles expects radians.
    m_rotation = glm::quat(glm::radians(eulerDegrees));
}

// Computes and returns the local transformation matrix using glm math.
glm::mat4 Transform::GetLocalMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);

    // Apply Translation, Rotation, then Scale.
    model = glm::translate(model, m_position);
    model *= glm::mat4_cast(m_rotation);
    model = glm::scale(model, m_scale);

    return model;
}
