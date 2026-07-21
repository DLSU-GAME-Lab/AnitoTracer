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

    if (m_mainCamera == nullptr)
    {
        m_mainCamera = camera.get();
    }

    AddComponentToObject(newObject, std::move(camera));
    AddComponentToObject(newObject, std::move(transform));

    return newObject;
}

bool HierarchyManager::GetMainCameraMatrices(glm::mat4& outViewMatrix, glm::mat4& outProjectionMatrix) {
    if (m_mainCamera != nullptr)
    {
        // Ensure the matrices are up-to-date with the current Transform data
        m_mainCamera->UpdateViewMatrix();
        m_mainCamera->UpdateProjectionMatrix();

        // Extract the required matrices for the rendering pipeline
        outViewMatrix = m_mainCamera->GetViewMatrix();
        outProjectionMatrix = m_mainCamera->GetProjectionMatrix();

        return true;
    }

    // Return false if no main camera has been assigned
    return false;
}

HierarchyObject* HierarchyManager::CreateModelObject(const std::string& name, const std::string& filepath) {
    Model* pModel = ModelManager::GetInstance().LoadModel(filepath); //

    HierarchyObject* newObject = CreateRootObject(name); //

    auto transform = std::make_unique<Transform>(); //
    auto modelComp = std::make_unique<ModelComponent>(pModel); 

    // 4. Attach components to the new object
    AddComponentToObject(newObject, std::move(transform)); 
    AddComponentToObject(newObject, std::move(modelComp)); 

    return newObject;
}

static void GatherModelsRecursive(HierarchyObject* obj, const glm::mat4& parentMatrix, std::vector<ModelRenderInstance>& outModels) {
    if (!obj) return;

    glm::mat4 currentWorldMatrix = parentMatrix;

    // If the object has a Transform component, multiply the parent matrix by the local matrix
    if (Transform* transform = obj->GetComponent<Transform>()) {
        currentWorldMatrix *= transform->GetLocalMatrix();
    }

    // If the object has a ModelComponent, extract the internal Model struct
    if (ModelComponent* modelComp = obj->GetComponent<ModelComponent>()) {
        // Retrieve the underlying Model struct pointer
        if (Model* pModel = modelComp->GetModel()) {
            outModels.push_back({ pModel, currentWorldMatrix });
        }
    }

    // Recursively process children to maintain hierarchical transforms
    for (const auto& child : obj->GetChildren()) {
        GatherModelsRecursive(child.get(), currentWorldMatrix, outModels);
    }
}

void HierarchyManager::GatherRenderModels(std::vector<ModelRenderInstance>& outModels) const
{
    // Clear any leftover data from the previous frame
    outModels.clear();

    // Start with an identity matrix for root-level objects
    glm::mat4 rootMatrix(1.0f);

    // Iterate through all root nodes managed by HierarchyManager
    for (const auto& rootNode : m_rootNodes) {
        GatherModelsRecursive(rootNode.get(), rootMatrix, outModels);
    }
}
