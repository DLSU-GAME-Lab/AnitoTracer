#pragma once

#include ANITO_SERIALIZATION_INCLUDES
#include ANITO_EVENT_INCLUDES

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>

#include "Components/ComponentBase.hpp"
#include "PropertyDrawers/componentbase_drawer.hpp" // should come after #include "Components/ComponentBase.hpp"

#include "Components/Transform.hpp"

#include "Organization/IInstanceManager.hpp"

class HierarchyObject : public gbe::ISerializable, public gbe::IInstanceManager<HierarchyObject>{
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

    // Core getters for object traversal and identification.
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    HierarchyObject::Ref GetParent() const { return m_parent; }
    const std::vector<std::unique_ptr<HierarchyObject>>& GetChildren() const { return m_children; }
    const std::vector<std::unique_ptr<ComponentBase>>& GetComponents() const { return m_components; }
    bool IsPrefabInstance() const { return !m_prefabAssetPath.empty(); }
    const std::string& GetPrefabAssetPath() const { return m_prefabAssetPath; }
    const std::unordered_map<std::string, std::string>& GetPrefabOverrides() const { return m_prefabOverrides; }
    std::vector<std::unique_ptr<HierarchyObject>>& MutableChildren() { return m_children; }
    void SetParent(HierarchyObject::Ref parent) { m_parent = parent; }
    std::string& MutablePrefabAssetPath() { return m_prefabAssetPath; }
    std::unordered_map<std::string, std::string>& MutablePrefabOverrides() { return m_prefabOverrides; }
    void ClearPrefabLink() { m_prefabAssetPath.clear(); m_prefabOverrides.clear(); }

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

    // Adds a component to this object and takes ownership.
    void AddComponent(std::unique_ptr<ComponentBase> component);

    // Removes a component by its raw pointer and returns ownership to the caller.
    std::unique_ptr<ComponentBase> RemoveComponent(ComponentBase* componentToRemove);


    //================//EVENTS//================//
    /**
     * @brief Dispatches an event to components on this object, with optional recursive propagation to children.
     * @param event The event payload struct.
     * @param recursive If true, propagates down through all child nodes.
     */
    template <typename TEvent>
    void DispatchEventData(const TEvent& event, bool recursive = false) {
        // 1. Dispatch to all local components attached to this object
        for (auto& component : m_components) {
            gbe::TriggerDispatcher::Dispatch(component.get(), event);
        }

        // 2. Recursively dispatch down child nodes if requested
        if (recursive) {
            std::vector<HierarchyObject::Ref> childrenToDispatch;
            childrenToDispatch.reserve(m_children.size());
            for (const auto& child : m_children) {
                if (child) {
                    childrenToDispatch.push_back(child->getRef());
                }
            }

            for (const HierarchyObject::Ref childRef : childrenToDispatch) {
                if (HierarchyObject* child = childRef.GetPtr()) {
                    child->DispatchEventData(event, true);
                }
            }
        }
    }

    /**
     * @brief In-place event construction helper.
     */
    template <typename TEvent, typename... Args>
    void DispatchEvent(bool recursive, Args&&... args) {
        TEvent event{ std::forward<Args>(args)... };
        DispatchEventData<TEvent>(event, recursive);
    }

private:
    std::string m_name;
    GBE_SERIALIZE_FIELD(m_name);
    HierarchyObject::Ref m_parent;
    std::vector<std::unique_ptr<HierarchyObject>> m_children;
    GBE_SERIALIZE_FIELD(m_children);

    std::vector<std::unique_ptr<ComponentBase>> m_components;
    void InitializeChildren(std::vector < std::unique_ptr<ComponentBase>>& target);
    GBE_SERIALIZE_FIELD_W_CB(m_components, std::bind_front(&HierarchyObject::InitializeChildren, this));

    std::string m_prefabAssetPath;
    GBE_SERIALIZE_FIELD(m_prefabAssetPath);

    std::unordered_map<std::string, std::string> m_prefabOverrides;
    GBE_SERIALIZE_FIELD(m_prefabOverrides);

    friend class HierarchyManager;

    inline void GBE_Init() {};
    GBE_GENERATE_SERIALIZER_CONSTRUCTOR_W_NAME(HierarchyObject, gbe::ISerializable, [this]() {return m_name; });
    GBE_DECLARE_INSTANCE_REF(HierarchyObject);
};

GBE_REGISTER_SERIALIZED_TYPE(HierarchyObject, HierarchyObject);