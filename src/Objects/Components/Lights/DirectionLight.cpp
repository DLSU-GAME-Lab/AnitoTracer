#include "DirectionLight.hpp"
#include "../../HierarchyObject.hpp"

// Calculates the world direction by multiplying the Transform's 
// quaternion rotation by the local direction vector.
glm::vec3 DirectionalLight::GetDirection() const {
    HierarchyObject::Ref owner = GetOwner();
    if (owner != nullptr) {
        if (const Transform* transform = owner.GetPtr()->GetTransform()) {
            glm::quat rotation = transform->GetRotation();
            return glm::normalize(rotation * m_localDirection);
        }
    }

    // Fallback to local direction if no transform is attached.
    return m_localDirection;
}
