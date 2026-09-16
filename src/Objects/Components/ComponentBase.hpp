#pragma once

#include ANITO_SERIALIZATION_INCLUDES
#include ANITO_EVENT_INCLUDES

#include <string>
#include <iostream>

#include "Organization/IInstanceManager.hpp"
#include "AssetPipeline.hpp"


// Forward declaration to avoid circular dependency
class HierarchyObject;

class ComponentBase : public gbe::ISerializable {
public:
    // Initializes the component with a name and an optional owner.
    ComponentBase(const std::string& name, gbe::IInstanceManager<HierarchyObject>::Ref owner = {})
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
    gbe::IInstanceManager<HierarchyObject>::Ref GetOwner() const { return m_owner; }

    // Sets or updates the owning HierarchyObject. This is the single point
    // both construction paths converge on (direct C++ construction with an
    // owner argument, and reflection-based construction via TypeRegistry::
    // Instantiate for the "Add Component" UI, which only runs the
    // SerializedData constructor chain and never the normal constructor
    // body). OnOwnerSet() lets derived components run owner-dependent setup
    // exactly once, regardless of which path created them.
    void SetOwner(gbe::IInstanceManager<HierarchyObject>::Ref owner) {
        m_owner = owner;
        if (m_owner.GetPtr()) {
            OnOwnerSet();
        }
    }

protected:
    std::string m_name;
    GBE_SERIALIZE_FIELD(m_name);

    gbe::IInstanceManager<HierarchyObject>::Ref m_owner;

    virtual inline void GBE_Init() {};

    // Called once a valid owner is assigned. Override to perform setup that
    // requires the owning HierarchyObject (e.g. reading its Transform).
    virtual void OnOwnerSet() {}

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(ComponentBase, gbe::ISerializable);
public:
    virtual std::string GetLabel() override;
};
