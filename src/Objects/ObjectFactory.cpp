#include "ObjectFactory.hpp"

// Assuming ModelManager exists based on original code usage
class ModelManager;

HierarchyObject::Ref ObjectFactory::CreateRootObjectWithTransform(const std::string& name) {
    // First, create the root object using the existing helper method.
    HierarchyObject::Ref newObject = HierarchyManager::GetInstance().CreateNewEmpty(name);

    // Instantiate the Transform component using the correct default constructor.
    // The constructor defaults to a nullptr owner and sets the name internally.
    auto transform = std::make_unique<Transform>();

    // Finally, attach the component to the newly created object.
    HierarchyManager::GetInstance().AddComponentToObject(newObject, std::move(transform));

    return newObject;
}

HierarchyObject::Ref ObjectFactory::CreateRootCameraObject(const std::string& name) {
    if (HierarchyManager::GetInstance().GetEditorCamera() != nullptr)
        return HierarchyManager::GetInstance().GetEditorCamera()->GetOwner();

    HierarchyObject::Ref newObject = HierarchyManager::GetInstance().CreateNewEmpty(name);

    auto transform = std::make_unique<Transform>();

    // Default root camera is editor-only; runtime camera is selected from GameCamera instances.
    auto camera = std::make_unique<EditorCamera>(transform.get(), newObject);

    HierarchyManager::GetInstance().AddComponentToObject(newObject, std::move(camera));
    HierarchyManager::GetInstance().AddComponentToObject(newObject, std::move(transform));

    return newObject;
}

HierarchyObject::Ref ObjectFactory::CreateModelObject(const std::string& name, const std::string& filepath) {
    Model* pModel = ModelManager::GetAssetByPath(filepath);

    HierarchyObject::Ref newObject = HierarchyManager::GetInstance().CreateNewEmpty(name);

    auto transform = std::make_unique<Transform>();
    auto modelComp = std::make_unique<ModelComponent>(pModel);

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
HierarchyObject::Ref ObjectFactory::CreatePointLightObject(const std::string& name) {
    // Leverage the existing method to create the root object and attach the Transform component
    HierarchyObject::Ref newObject = CreateRootObjectWithTransform(name);

    // Instantiate the DirectionalLight component, passing the owner object
    auto _light = std::make_unique<PointLight>(newObject);

    // Attach the light component to the newly created object
    HierarchyManager::GetInstance().AddComponentToObject(newObject, std::move(_light));

    return newObject;
}

HierarchyObject::Ref ObjectFactory::CreateSpherePrimitive(const std::string& name) {
    return CreateModelObject(name, "Assets/Primitives/sphere.obj");
}

HierarchyObject::Ref ObjectFactory::CreateCubePrimitive(const std::string& name) {
    return CreateModelObject(name, "Assets/Primitives/cube.obj");
}