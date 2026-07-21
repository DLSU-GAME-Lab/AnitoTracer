#include "HierarchyManager.hpp"

HierarchyObject* HierarchyManager::CreateRootObject(const std::string& name) {
    auto root = std::make_unique<HierarchyObject>(name);
    m_rootNodes.push_back(std::move(root));
    return m_rootNodes.back().get();
}

HierarchyObject* HierarchyManager::AddRootObject(std::unique_ptr<HierarchyObject> rootObj) {
    if (!rootObj) return nullptr;

    m_rootNodes.push_back(std::move(rootObj));
    return m_rootNodes.back().get();
}

std::unique_ptr<HierarchyObject> HierarchyManager::RemoveRootObject(HierarchyObject* rootToRemove) {
    for (auto it = m_rootNodes.begin(); it != m_rootNodes.end(); ++it) {
        if (it->get() == rootToRemove) {
            std::unique_ptr<HierarchyObject> detachedRoot = std::move(*it);
            m_rootNodes.erase(it);
            return detachedRoot;
        }
    }
    return nullptr;
}

void HierarchyManager::AddComponentToObject(HierarchyObject* object, std::unique_ptr<ComponentBase> component) {
    if (!object || !component) return;

    // Assign the owner before moving the component into the object's vector.
    component->SetOwner(object);

    // Accessing private member m_components requires friend class declaration.
    object->m_components.push_back(std::move(component));
}

std::unique_ptr<ComponentBase> HierarchyManager::RemoveComponentFromObject(HierarchyObject* object, ComponentBase* componentToRemove) {
    if (!object || !componentToRemove) return nullptr;

    for (auto it = object->m_components.begin(); it != object->m_components.end(); ++it) {
        if (it->get() == componentToRemove) {
            std::unique_ptr<ComponentBase> detachedComponent = std::move(*it);

            // Clear the owner pointer as it is no longer attached.
            detachedComponent->SetOwner(nullptr);
            object->m_components.erase(it);

            return detachedComponent;
        }
    }
    return nullptr;
}

HierarchyObject* HierarchyManager::CreateRootObjectWithTransform(const std::string& name)
{
    // First, create the root object using the existing helper method.
    HierarchyObject* newObject = CreateRootObject(name);

    // Instantiate the Transform component using the correct default constructor.
    // The constructor defaults to a nullptr owner and sets the name internally.
    auto transform = std::make_unique<Transform>();

    // Finally, attach the component to the newly created object.
    AddComponentToObject(newObject, std::move(transform));

    return newObject;
}

HierarchyObject* HierarchyManager::CreateRootCameraObject(const std::string& name)
{
    HierarchyObject* newObject = CreateRootObject(name);

    auto transform = std::make_unique<Transform>();

    // Fix: Use .get() to extract the raw pointer from the unique_ptr for the constructor
    auto camera = std::make_unique<CameraComponent>(transform.get(), newObject);

    AddComponentToObject(newObject, std::move(camera));
    AddComponentToObject(newObject, std::move(transform));

    return newObject;
}
