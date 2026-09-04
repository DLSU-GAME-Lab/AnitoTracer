#include "HierarchyObject.hpp"
#include "HierarchyManager.hpp"

// Shortcut implementation returning a modifiable pointer to Transform
Transform* HierarchyObject::GetTransform() {
    return GetComponent<Transform>();
}

// Const shortcut implementation
const Transform* HierarchyObject::GetTransform() const {
    return GetComponent<Transform>();
}

void HierarchyObject::AddComponent(std::unique_ptr<ComponentBase> component)
{
    if (!component) return;

    // Assign this object as the owner using getRef() inherited from IInstanceManager.
    component->SetOwner(this->getRef());

    // Take ownership of the component by moving it into the vector.
    m_components.push_back(std::move(component));
}

void HierarchyObject::InitializeChildren(std::vector<std::unique_ptr<ComponentBase>>& target) {
    for (auto& newchild : target)
    {
        newchild.get()->SetOwner(this);
    }
}

std::unique_ptr<ComponentBase> HierarchyObject::RemoveComponent(ComponentBase * componentToRemove)
{
    if (!componentToRemove) return nullptr;

    for (auto it = m_components.begin(); it != m_components.end(); ++it) {
        if (it->get() == componentToRemove) {
            std::unique_ptr<ComponentBase> detachedComponent = std::move(*it);

            detachedComponent->SetOwner(nullptr);

            m_components.erase(it);

            return detachedComponent;
        }
    }

    return nullptr;
}
