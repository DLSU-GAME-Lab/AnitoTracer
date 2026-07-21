#include "Camera.hpp"

CameraComponent::CameraComponent(Transform* transform, HierarchyObject* owner)
    : ComponentBase("CameraComponent", owner), m_transform(transform) {
    
    if (!transform || !owner) std::cerr << "Invalid or null transform or owner pointer" << std::endl;

}

void CameraComponent::UpdateViewMatrix() {
    if (!m_transform) return;

    // Fetch the current position and rotation from the attached Transform
    glm::vec3 position = m_transform->GetPosition();
    glm::quat rotation = m_transform->GetRotation();

    // Calculate the forward and up vectors using the quaternion.
    // In a left-handed system, forward is +Z and up is +Y.
    glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 up = rotation * glm::vec3(0.0f, 1.0f, 0.0f);

    // The target is simply the current position plus the forward direction vector
    glm::vec3 target = position + forward;

    m_ViewMatrix = glm::lookAt(position, target, up);
}

void CameraComponent::UpdateProjectionMatrix() {
    m_ProjMatrix = glm::perspective(glm::radians(m_FOV), m_Aspect, m_NearZ, m_FarZ);
}

glm::mat4 CameraComponent::GetViewProjectionMatrix() const {
    return m_ProjMatrix * m_ViewMatrix;
}
