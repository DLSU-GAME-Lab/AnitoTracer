#pragma once

#include <string>
#include <iostream>

// Forward declaration to avoid circular dependency
class HierarchyObject;

class ComponentBase {
public:
    // Initializes the component with a name and an optional owner.
    ComponentBase(const std::string& name, HierarchyObject* owner = nullptr)
        : m_name(name), m_owner(owner) {}

    // A virtual destructor is critical for base classes to ensure 
    // derived class destructors are called correctly.
    virtual ~ComponentBase() = default;

    // Delete copy constructor and assignment operator to prevent object slicing.
    ComponentBase(const ComponentBase&) = delete;
    ComponentBase& operator=(const ComponentBase&) = delete;

    // Allow moving for container compatibility.
    ComponentBase(ComponentBase&&) = default;
    ComponentBase& operator=(ComponentBase&&) = default;

    // Core getters for the component data.
    const std::string& GetName() const { return m_name; }
    HierarchyObject* GetOwner() const { return m_owner; }

    // Sets or updates the owning HierarchyObject.
    void SetOwner(HierarchyObject* owner) { m_owner = owner; }

protected:
    std::string m_name;
    HierarchyObject* m_owner;
};