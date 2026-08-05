#include "PointLight.hpp"
#include "../../HierarchyObject.hpp"

// Retrieves the world position by querying the owner's Transform component.
glm::vec3 PointLight::GetPosition() const {
    HierarchyObject::Ref owner = GetOwner();
    if (owner != nullptr) {
        if (const Transform* transform = owner.GetPtr()->GetTransform()) {
            // Assuming the Transform class has a GetPosition() method
            return transform->GetPosition();
        }
    }

    // Fallback to the origin if no transform is attached.
    return glm::vec3(0.0f, 0.0f, 0.0f);
}