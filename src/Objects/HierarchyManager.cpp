#include "HierarchyManager.hpp"
#include "HierarchyManager.hpp"

#include "../AppConfig.hpp"
#include "ObjectFactory.hpp"

CameraComponent* HierarchyManager::GetMainCamera() const {
    return gbe::IInstanceManager<GameCamera>::getOldest();
}

HierarchyObject::Ref HierarchyManager::AddRootObject(std::unique_ptr<HierarchyObject> rootObj) {
    if (!rootObj) return nullptr;

    m_rootNodes.push_back(std::move(rootObj));

    return  m_rootNodes.back().get();
}

std::unique_ptr<HierarchyObject> HierarchyManager::RemoveRootObject(HierarchyObject::Ref rootToRemove) {
    for (auto it = m_rootNodes.begin(); it != m_rootNodes.end(); ++it) {
        if (it->get() == rootToRemove.GetPtr()) {
            std::unique_ptr<HierarchyObject> detachedRoot = std::move(*it);
            m_rootNodes.erase(it);
            return detachedRoot;
        }
    }
    return nullptr;
}

void HierarchyManager::QueueObjectDeletion(HierarchyObject::Ref object) {
    if (!object) return;

    const auto objectId = object.GetID();
    for (const auto queuedId : m_deferredDeletionIds) {
        if (queuedId == objectId) return;
    }

    m_deferredDeletionIds.push_back(objectId);
}

bool HierarchyManager::ReparentObject(HierarchyObject::Ref object, HierarchyObject::Ref parent) {
    HierarchyObject* objectPtr = object.GetPtr();
    HierarchyObject* parentPtr = parent.GetPtr();
    if (!objectPtr || objectPtr == parentPtr) return false;

    // A node cannot become a child of itself or one of its descendants.
    for (HierarchyObject* ancestor = parentPtr; ancestor != nullptr;) {
        if (ancestor == objectPtr) return false;
        ancestor = ancestor->GetParent().GetPtr();
    }

    if (objectPtr->GetParent() == parent) return true;

    std::unique_ptr<HierarchyObject> detachedObject;
    if (HierarchyObject::Ref oldParent = objectPtr->GetParent()) {
        detachedObject = oldParent.GetPtr()->RemoveChild(object);
    }
    else {
        detachedObject = RemoveRootObject(object);
    }

    if (!detachedObject) return false;

    if (parentPtr) {
        parentPtr->AddChild(std::move(detachedObject));
    }
    else {
        AddRootObject(std::move(detachedObject));
    }
    return true;
}

bool HierarchyManager::CopyObject(HierarchyObject::Ref object) {
    HierarchyObject* objectPtr = object.GetPtr();
    if (!objectPtr) return false;

    m_copiedObject = objectPtr->Serialize();
    for (auto it = m_copiedObject.serialized_variables.begin();
         it != m_copiedObject.serialized_variables.end();) {
        const std::string& key = it->first;
        if (key == "m_guid" ||
            (key.size() > 7 && key.compare(key.size() - 7, 7, ".m_guid") == 0)) {
            it = m_copiedObject.serialized_variables.erase(it);
        }
        else {
            ++it;
        }
    }
    m_copiedObject.label = "HierarchyObjectClipboard";
    m_hasCopiedObject = true;
    return true;
}

HierarchyObject::Ref HierarchyManager::PasteObject(HierarchyObject::Ref parent) {
    if (!m_hasCopiedObject) return nullptr;

    gbe::SerializedData pasteData = m_copiedObject;
    gbe::ISerializable* rawObject = gbe::TypeRegistry::Instantiate(
        typeid(HierarchyObject).name(), pasteData);
    auto* pastedObject = dynamic_cast<HierarchyObject*>(rawObject);
    if (!pastedObject) {
        delete rawObject;
        return nullptr;
    }

    pastedObject->Deserialize(pasteData);

    std::unique_ptr<HierarchyObject> ownedObject(pastedObject);
    if (parent) {
        return parent.GetPtr()->AddChild(std::move(ownedObject));
    }

    return AddRootObject(std::move(ownedObject));
}

size_t HierarchyManager::CommitDeferredDeletions() {
    std::vector<gbe::IInstanceManager<HierarchyObject>::IdType> pendingDeletions;
    pendingDeletions.swap(m_deferredDeletionIds);

    size_t removedCount = 0;
    for (const auto objectId : pendingDeletions) {
        HierarchyObject::Ref object(objectId);
        HierarchyObject* objectPtr = object.GetPtr();
        if (!objectPtr) continue;

        if (HierarchyObject::Ref parent = objectPtr->GetParent()) {
            if (parent.GetPtr()->RemoveChild(object)) {
                ++removedCount;
            }
        }
        else if (RemoveRootObject(object)) {
            ++removedCount;
        }
    }

    return removedCount;
}

void HierarchyManager::AddComponentToObject(HierarchyObject::Ref object, std::unique_ptr<ComponentBase> component) {
    if (!object || !component) return;
    //Moved it to object
    object.GetPtr()->AddComponent(std::move(component));
}

std::unique_ptr<ComponentBase> HierarchyManager::RemoveComponentFromObject(HierarchyObject::Ref object, ComponentBase* componentToRemove) {
    if (!object || !componentToRemove) return nullptr;

    //Moved it to object
    return object.GetPtr()->RemoveComponent(componentToRemove);
}

bool HierarchyManager::GetMainCameraMatrices(glm::mat4& outViewMatrix, glm::mat4& outProjectionMatrix) {
    CameraComponent* activeCamera = nullptr;
    if (AppConfig::release) {
        activeCamera = GetMainCamera();
    }
    else {
        activeCamera = GetEditorCamera();
        if (!activeCamera) {
            activeCamera = GetMainCamera();
        }
    }

    if (activeCamera != nullptr)
    {
        // Ensure the matrices are up-to-date with the current Transform data
        activeCamera->UpdateViewMatrix();
        activeCamera->UpdateProjectionMatrix();

        // Extract the required matrices for the rendering pipeline
        outViewMatrix = activeCamera->GetViewMatrix();
        outProjectionMatrix = activeCamera->GetProjectionMatrix();

        return true;
    }

    // Return false if no main camera has been assigned
    return false;
}

bool HierarchyManager::GetEditorCameraMatrices(glm::mat4& outViewMatrix, glm::mat4& outProjectionMatrix)
{
    CameraComponent* editorCamera = GetEditorCamera();

    if (editorCamera!= nullptr) {

        // Ensure the matrices are up-to-date with the current Transform data
        editorCamera->UpdateViewMatrix();
        editorCamera->UpdateProjectionMatrix();

        // Extract the required matrices for the rendering pipeline
        outViewMatrix = editorCamera->GetViewMatrix();
        outProjectionMatrix = editorCamera->GetProjectionMatrix();

        return true;
    }

    return false;
}

static void GatherModelsRecursive(HierarchyObject::Ref obj, const glm::mat4& parentMatrix, std::vector<ModelRenderInstance>& outModels) {
    if (!obj) return;

    glm::mat4 currentWorldMatrix = parentMatrix;

    // If the object has a Transform component, multiply the parent matrix by the local matrix
    if (Transform* transform = obj.GetPtr()->GetComponent<Transform>()) {
        currentWorldMatrix *= transform->GetLocalMatrix();
    }

    // If the object has a ModelComponent, extract the internal Model struct
    if (ModelComponent* modelComp = obj.GetPtr()->GetComponent<ModelComponent>()) {
        // Retrieve the underlying Model struct pointer
        if (Model* pModel = modelComp->GetModel().Get()) {
            ModelRenderInstance instance;
            instance.ModelData = pModel;
            instance.WorldTransform = currentWorldMatrix;
            instance.OwnerID = obj.GetID();

            // ADDED: Sort submeshes based on material transparency
            for (uint32_t i = 0; i < pModel->SubMeshes.size(); ++i) {
                uint32_t matIndex = pModel->SubMeshes[i].MaterialIndex;
                if (pModel->PBRMaterials[matIndex].IsTransparent) {
                    instance.TransparentSubmeshIndices.push_back(i);
                }
                else {
                    instance.OpaqueSubmeshIndices.push_back(i);
                }
            }

            outModels.push_back(instance);
        }
    }

    // Recursively process children to maintain hierarchical transforms
    for (const auto& child : obj.GetPtr()->GetChildren()) {
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

static void GatherLightsRecursive(HierarchyObject::Ref obj, const glm::mat4& parentMatrix, Diligent::LightConstants& outLights) {
    if (!obj) return;

    glm::mat4 currentWorldMatrix = parentMatrix;

    // Apply local transform to the accumulated matrix
    if (Transform* transform = obj.GetPtr()->GetComponent<Transform>()) {
        currentWorldMatrix *= transform->GetLocalMatrix();
    }

    // Process Directional Lights using Master's custom component!
    if (DirectionalLight* dirLight = obj.GetPtr()->GetComponent<DirectionalLight>()) {
        if (outLights.NumDirLights < Diligent::MAX_DIR_LIGHTS) {
            // Retrieve the direction already calculated with the Transform's quaternion rotation
            glm::vec3 worldDir = dirLight->GetDirection();

            Diligent::DirectionalLightData& lightData = outLights.DirLights[outLights.NumDirLights];
            lightData.Direction = glm::vec4(worldDir, 0.0f);

            // Pack color and intensity using Getters from LightBase
            lightData.Color = glm::vec4(dirLight->GetColor(), dirLight->GetIntensity());

            outLights.NumDirLights++;
        }
    }

    // Process Point Lights (Assuming you make a PointLight class inheriting LightBase next!)
    if (PointLight* pointLight = obj.GetPtr()->GetComponent<PointLight>()) {
        if (outLights.NumPointLights < Diligent::MAX_POINT_LIGHTS) {
            // Extract the absolute world position from the 4th column of the world matrix
            glm::vec3 worldPos = glm::vec3(currentWorldMatrix[3]);

            Diligent::PointLightData& lightData = outLights.PointLights[outLights.NumPointLights];
            lightData.Position = glm::vec4(worldPos, 1.0f);

            // Reusing LightBase getters
            lightData.Color = glm::vec4(pointLight->GetColor(), pointLight->GetIntensity());
            lightData.Range = pointLight->GetRange();

            outLights.NumPointLights++;
        }
    }
    
    // Recursively process children
    for (const auto& child : obj.GetPtr()->GetChildren()) {
        GatherLightsRecursive(child.get(), currentWorldMatrix, outLights);
    }
}

void HierarchyManager::GatherLightData(Diligent::LightConstants& outLights) const {
    // Reset light counts for the current frame
    outLights.NumDirLights = 0;
    outLights.NumPointLights = 0;

    // Pass the camera position to the light constants for specular calculations
    glm::vec3 cameraPos;
    if (GetMainCameraPosition(cameraPos)) {
        outLights.CameraPos = glm::vec4(cameraPos, 1.0f);
    }

    glm::mat4 rootMatrix(1.0f);

    // Iterate through all root nodes managed by HierarchyManager
    for (const auto& rootNode : m_rootNodes) {
        GatherLightsRecursive(rootNode.get(), rootMatrix, outLights);
    }
}

bool HierarchyManager::GetMainCameraPosition(glm::vec3& outPosition) const {
    CameraComponent* activeCamera = nullptr;
    if (AppConfig::release) {
        activeCamera = GetMainCamera();
    }
    else {
        activeCamera = GetEditorCamera();
        if (!activeCamera) {
            activeCamera = GetMainCamera();
        }
    }

    if (activeCamera != nullptr) {
        if (HierarchyObject::Ref camOwner = activeCamera->GetOwner()) {
            if (Transform* camTransform = camOwner.GetPtr()->GetComponent<Transform>()) {
                outPosition = camTransform->GetPosition();
                return true;
            }
        }
    }

    return false;
}

gbe::SerializedData HierarchyManager::Serialize()
{
    return gbe::ISerializable::Serialize();
}

void HierarchyManager::Deserialize(gbe::SerializedData& data)
{
    gbe::EventSystem::DispatchTo(
        EVENT_ONSCENELOAD,
        std::make_unique<SceneLoadArgs>(data.label)
    );

    gbe::ISerializable::Deserialize(data);
    EnsureEditorCameraExists();
}

void HierarchyManager::EnsureEditorCameraExists()
{
    if (GetEditorCamera() != nullptr) {
        return;
    }

    HierarchyObject::Ref editorCamObject = ObjectFactory::GetInstance().CreateRootCameraObject("Main Camera");
    if (editorCamObject && editorCamObject.GetPtr()->GetTransform()) {
        editorCamObject.GetPtr()->GetTransform()->SetPosition(glm::vec3(0.0f, 0.0f, -10.0f));
    }
}

void HierarchyManager::LoadScene(std::filesystem::path filepath)
{
    m_sceneFile = filepath;
    this->DeserializeFromFile(filepath);
    EnsureEditorCameraExists();
}

void HierarchyManager::QuickSave()
{
    if(m_sceneFile.empty()){
        return;
    }
    
    this->SerializeToFile(m_sceneFile);
}
