#include "HierarchyObject.hpp"

// Adds an existing child object and takes ownership.
// Returns a raw pointer to the added child for immediate access.
HierarchyObject::Ref HierarchyObject::AddChild(std::unique_ptr<HierarchyObject> child) {
    if (!child) return nullptr;

    child->m_parent = this;
    m_children.push_back(std::move(child));
    return m_children.back().get()->getRef();
}

// Helper method to instantiate and add a child directly by name.
HierarchyObject::Ref HierarchyObject::CreateChild(const std::string& childName) {
    auto child = std::make_unique<HierarchyObject>(childName);
    return AddChild(std::move(child));
}

// Removes a child by its exact pointer address.
// Returns the unique_ptr, transferring ownership back to the caller.
std::unique_ptr<HierarchyObject> HierarchyObject::RemoveChild(HierarchyObject::Ref childToRemove) {
    for (auto it = m_children.begin(); it != m_children.end(); ++it) {
        if (it->get() == childToRemove.GetPtr()) {
            std::unique_ptr<HierarchyObject> detachedChild = std::move(*it);
            detachedChild->m_parent = nullptr;
            m_children.erase(it);
            return detachedChild;
        }
    }
    return nullptr;
}

// Shortcut implementation returning a modifiable pointer to Transform
Transform* HierarchyObject::GetTransform() {
    return GetComponent<Transform>();
}

// Const shortcut implementation
const Transform* HierarchyObject::GetTransform() const {
    return GetComponent<Transform>();
}
