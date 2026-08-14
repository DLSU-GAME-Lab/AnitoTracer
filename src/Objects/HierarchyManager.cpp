#include "HierarchyManager.hpp"

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
    if (GetMainCamera() != nullptr)
    {
        // Ensure the matrices are up-to-date with the current Transform data
        GetMainCamera()->UpdateViewMatrix();
        GetMainCamera()->UpdateProjectionMatrix();

        // Extract the required matrices for the rendering pipeline
        outViewMatrix = GetMainCamera()->GetViewMatrix();
        outProjectionMatrix = GetMainCamera()->GetProjectionMatrix();

        return true;
    }

    // Return false if no main camera has been assigned
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
            outModels.push_back({ pModel, currentWorldMatrix, obj.GetID() });
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
    if (GetMainCamera() != nullptr) {
        if (HierarchyObject::Ref camOwner = GetMainCamera()->GetOwner()) {
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
    return gbe::SerializedData();
}

void HierarchyManager::Deserialize(gbe::SerializedData& data)
{
    gbe::EventSystem::DispatchTo(
        EVENT_ONSCENELOAD,
        std::make_unique<SceneLoadArgs>(data.label)
    );

    gbe::ISerializable::Deserialize(data);
}
