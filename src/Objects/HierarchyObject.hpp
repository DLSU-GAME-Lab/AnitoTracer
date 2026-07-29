#pragma once

#include ANITOSERIALIZATIONINCLUDE

#include <string>
#include <vector>
#include <memory>

#include "Components/ComponentBase.hpp"
#include "Components/Transform.hpp"

class HierarchyObject : public gbe::ISerializable{
public:
    // Initializes the object with a specific name.
    HierarchyObject(const std::string& name)
        : m_name(name), m_parent(nullptr) {}

    // Default destructor.
    ~HierarchyObject() = default;

    // Delete copy constructor and assignment operator to enforce unique ownership.
    HierarchyObject(const HierarchyObject&) = delete;
    HierarchyObject& operator=(const HierarchyObject&) = delete;

    // Allow moving for container compatibility.
    HierarchyObject(HierarchyObject&&) = default;
    HierarchyObject& operator=(HierarchyObject&&) = default;

    // Adds an existing child object and takes ownership.
    // Returns a raw pointer to the added child for immediate access.
    HierarchyObject* AddChild(std::unique_ptr<HierarchyObject> child);

    // Helper method to instantiate and add a child directly by name.
    HierarchyObject* CreateChild(const std::string& childName);

    // Removes a child by its exact pointer address.
    // Returns the unique_ptr, transferring ownership back to the caller.
    std::unique_ptr<HierarchyObject> RemoveChild(HierarchyObject* childToRemove);

    // Core getters for object traversal and identification.
    const std::string& GetName() const { return m_name; }
    HierarchyObject* GetParent() const { return m_parent; }
    const std::vector<std::unique_ptr<HierarchyObject>>& GetChildren() const { return m_children; }
    const std::vector<std::unique_ptr<ComponentBase>>& GetComponents() const { return m_components; }

    /**
     * @brief Generic method to search for and return an attached component of type T.
     * @return Pointer to component of type T if attached; nullptr otherwise.
     */
    template <typename T>
    T* GetComponent() {
        for (auto& component : m_components) {
            if (T* casted = dynamic_cast<T*>(component.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    // Const overload for non-modifiable access when working with const HierarchyObjects
    template <typename T>
    const T* GetComponent() const {
        for (auto& component : m_components) {
            if (const T* casted = dynamic_cast<const T*>(component.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    Transform* GetTransform();
    const Transform* GetTransform() const;

private:
    std::string m_name;
    HierarchyObject* m_parent;
    std::vector<std::unique_ptr<HierarchyObject>> m_children;
    GBE_SERIALIZE_FIELD(m_children);

    std::vector<std::unique_ptr<ComponentBase>> m_components;
    GBE_SERIALIZE_FIELD(m_components);

    friend class HierarchyManager;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(HierarchyObject, gbe::ISerializable);
};

GBE_REGISTER_SERIALIZED_TYPE(HierarchyObject);