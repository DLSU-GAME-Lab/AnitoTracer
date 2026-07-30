#include "ObjectFactory.hpp"

// Assuming ModelManager exists based on original code usage
class ModelManager;

HierarchyObject::Ref ObjectFactory::CreateRootObject(const std::string& name) {
    auto root = std::make_unique<HierarchyObject>(name);
    return HierarchyManager::GetInstance().AddRootObject(std::move(root));
}

HierarchyObject::Ref ObjectFactory::CreateRootObjectWithTransform(const std::string& name) {
    // First, create the root object using the existing helper method.
    HierarchyObject::Ref newObject = CreateRootObject(name);

    // Instantiate the Transform component using the correct default constructor.
    // The constructor defaults to a nullptr owner and sets the name internally.
    auto transform = std::make_unique<Transform>();

    // Finally, attach the component to the newly created object.
    HierarchyManager::GetInstance().AddComponentToObject(newObject, std::move(transform));

    return newObject;
}

HierarchyObject::Ref ObjectFactory::CreateRootCameraObject(const std::string& name) {
    if (HierarchyManager::GetInstance().GetMainCamera() != nullptr)
        return HierarchyManager::GetInstance().GetMainCamera()->GetOwner();

    HierarchyObject::Ref newObject = CreateRootObject(name);

    auto transform = std::make_unique<Transform>();

    // Fix: Use .get() to extract the raw pointer from the unique_ptr for the constructor
    auto camera = std::make_unique<CameraComponent>(transform.get(), newObject);

    HierarchyManager::GetInstance().AddComponentToObject(newObject, std::move(camera));
    HierarchyManager::GetInstance().AddComponentToObject(newObject, std::move(transform));

    return newObject;
}

HierarchyObject::Ref ObjectFactory::CreateModelObject(const std::string& name, const std::string& filepath) {
    Model* pModel = ModelManager::GetInstance().LoadModel(filepath);

    HierarchyObject::Ref newObject = CreateRootObject(name);

    auto transform = std::make_unique<Transform>();
    auto modelComp = std::make_unique<ModelComponent>(pModel);

    // 4. Attach components to the new object
    HierarchyManager::GetInstance().AddComponentToObject(newObject, std::move(transform));
    HierarchyManager::GetInstance().AddComponentToObject(newObject, std::move(modelComp));

    return newObject;
}

HierarchyObject::Ref ObjectFactory::CreateDirectionalLightObject(const std::string& name) {
    // Leverage the existing method to create the root object and attach the Transform component
    HierarchyObject::Ref newObject = CreateRootObjectWithTransform(name);

    // Instantiate the DirectionalLight component, passing the owner object
    auto dirLight = std::make_unique<DirectionalLight>(newObject);

    // Attach the light component to the newly created object
    HierarchyManager::GetInstance().AddComponentToObject(newObject, std::move(dirLight));

    return newObject;
}