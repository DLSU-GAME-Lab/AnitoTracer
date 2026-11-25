#include "HierarchyManager.hpp"
#include "HierarchyManager.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "../AppConfig.hpp"
#include "../AppState.hpp"
#include "../Common/LightUnits.hpp"
#include "../Asset/ProjectSettings.hpp"
#include "ObjectFactory.hpp"
#include "HierarchyFeatures/PrefabFeature.hpp"

namespace {
    std::filesystem::path NormalizePath(const std::filesystem::path& path) {
        std::error_code error;
        const auto absolute = std::filesystem::absolute(path, error);
        return error ? path.lexically_normal() : absolute.lexically_normal();
    }

    // RAII helper so every mutating HierarchyManager entry point records a single
    // undo entry regardless of its return paths, and composes cleanly when one
    // mutator calls another (e.g. ObjectFactory building up a multi-component object).
    struct UndoScope {
        UndoScope() { HierarchyManager::GetInstance().BeginUndoableAction(); }
        ~UndoScope() { HierarchyManager::GetInstance().EndUndoableAction(); }
    };

    // A node that stands for a submesh of its parent's model. It belongs to that object: it is made, kept and removed by
    // SyncSubMeshNodes, so it can't be deleted, copied or moved elsewhere on its own, unless it no longer stands for any
    // submesh (the model changed), when it is an ordinary object again.
    bool IsLiveSubMeshNode(const HierarchyObject* object) {
        if (!object) return false;
        const SubMeshComponent* part = object->GetComponent<SubMeshComponent>();
        return part && !part->IsOrphaned();
    }

    // What the node for submesh `index` is called until it is renamed
    std::string SubMeshNodeName(const SubMesh& submesh, size_t index) {
        return submesh.Name.empty() ? "Submesh " + std::to_string(index) : submesh.Name;
    }

    // Settles the submesh nodes among the children of `object` with the model it holds now (see SyncSubMeshNodes)
    void SyncSubMeshChildrenOf(HierarchyObject* object) {
        ModelComponent* modelComponent = object->GetComponent<ModelComponent>();

        // A model with several submeshes gets a node for each. When the model is set but can't be had right now (loading,
        // being reloaded) nothing is known, and nothing is changed: its nodes may hold work.
        const Model* model = nullptr;
        size_t wanted = 0;
        if (modelComponent && modelComponent->HasModel()) {
            model = modelComponent->GetModel().Get();
            if (!model) return;
            if (model->SubMeshes.size() > 1) wanted = model->SubMeshes.size();
        }

        auto& children = object->MutableChildren();

        // Which child stands for which submesh. A child with an index that doesn't exist, or one that is already taken, is stale.
        std::vector<HierarchyObject*> nodeAt(wanted, nullptr);
        std::vector<const HierarchyObject*> stale;
        for (const auto& child : children) {
            const SubMeshComponent* part = child ? child->GetComponent<SubMeshComponent>() : nullptr;
            if (!part) continue;

            const int index = part->GetSubMeshIndex();
            if (index >= 0 && static_cast<size_t>(index) < wanted && !nodeAt[static_cast<size_t>(index)]) {
                nodeAt[static_cast<size_t>(index)] = child.get();
            }
            else {
                stale.push_back(child.get());
            }
        }

        // A stale node is removed, unless it holds something (overrides, components, children): that is left for the user to decide
        if (!stale.empty()) {
            children.erase(std::remove_if(children.begin(), children.end(), [&](const std::unique_ptr<HierarchyObject>& child) {
                if (std::find(stale.begin(), stale.end(), child.get()) == stale.end()) return false;
                return child->GetComponent<SubMeshComponent>()->IsPristine();
            }), children.end());
        }

        // The missing nodes
        for (size_t i = 0; i < wanted; ++i) {
            if (nodeAt[i]) continue;

            const SubMesh& submesh = model->SubMeshes[i];
            auto node = std::make_unique<HierarchyObject>(SubMeshNodeName(submesh, i));
            node->SetParent(object->getRef());
            node->AddComponent(std::make_unique<Transform>());
            node->AddComponent(std::make_unique<SubMeshComponent>(static_cast<int>(i), submesh.Name));
            nodeAt[i] = node.get();
            children.push_back(std::move(node));
        }

        // The nodes of the submeshes come first, in the order of the submeshes, then the object's other children
        const auto rank = [&](const std::unique_ptr<HierarchyObject>& child) {
            const SubMeshComponent* part = child ? child->GetComponent<SubMeshComponent>() : nullptr;
            if (!part) return std::numeric_limits<size_t>::max();
            const int index = part->GetSubMeshIndex();
            const bool live = index >= 0 && static_cast<size_t>(index) < wanted && nodeAt[static_cast<size_t>(index)] == child.get();
            return live ? static_cast<size_t>(index) : std::numeric_limits<size_t>::max();
        };
        const auto before = [&](const std::unique_ptr<HierarchyObject>& a, const std::unique_ptr<HierarchyObject>& b) { return rank(a) < rank(b); };
        if (!std::is_sorted(children.begin(), children.end(), before)) {
            std::stable_sort(children.begin(), children.end(), before);
        }

        // What each node is called by the model, and where it stands: at the middle of its part, as it is in the model, not turned
        // or scaled. What is attached to the node (a light, a particle system) then stays with the part. The geometry is not
        // moved by it: it is baked into the model.
        for (size_t i = 0; i < wanted; ++i) {
            HierarchyObject* node = nodeAt[i];
            const SubMesh& submesh = model->SubMeshes[i];

            if (SubMeshComponent* part = node->GetComponent<SubMeshComponent>()) {
                if (part->GetSubMeshName() != submesh.Name) {
                    part->SetSubMesh(static_cast<int>(i), submesh.Name);
                }
            }

            Transform* transform = node->GetTransform();
            if (!transform) {
                node->AddComponent(std::make_unique<Transform>());
                transform = node->GetTransform();
            }
            const float3 center = submesh.Center();
            const glm::vec3 pivot(center.x, center.y, center.z);
            if (transform->GetLocalPosition() != pivot) transform->SetPosition(pivot);
            if (transform->GetEulerAnglesDegrees() != glm::vec3(0.0f)) transform->SetEulerAnglesDegrees(glm::vec3(0.0f));
            if (transform->GetLocalScale() != glm::vec3(1.0f)) transform->SetScale(glm::vec3(1.0f));
        }
    }

    void SyncSubMeshNodesOf(HierarchyObject* object) {
        if (!object) return;

        SyncSubMeshChildrenOf(object);

        // Indexed, since settling the nodes may have changed the vector (the new nodes have nothing to settle)
        auto& children = object->MutableChildren();
        for (size_t i = 0; i < children.size(); ++i) {
            SyncSubMeshNodesOf(children[i].get());
        }
    }
}

void HierarchyManager::SyncSubMeshNodes() {
    for (size_t i = 0; i < m_rootNodes.size(); ++i) {
        SyncSubMeshNodesOf(m_rootNodes[i].get());
    }
}

// Play mode is discarded on stop (scene reload), so no undo bookkeeping while playing.
void HierarchyManager::BeginUndoableAction() {
    if (AppState::isPlaying) return;
    gbe::UndoRedoManager::GetInstance().BeginAction(GetGUID());
}

void HierarchyManager::EndUndoableAction() {
    if (AppState::isPlaying) return;
    gbe::UndoRedoManager::GetInstance().EndAction();
}

void HierarchyManager::Undo() {
    if (AppState::isPlaying) return;
    gbe::UndoRedoManager::GetInstance().Undo();
}

void HierarchyManager::Redo() {
    if (AppState::isPlaying) return;
    gbe::UndoRedoManager::GetInstance().Redo();
}

bool HierarchyManager::CanUndo() const {
    return !AppState::isPlaying && gbe::UndoRedoManager::GetInstance().CanUndo();
}

bool HierarchyManager::CanRedo() const {
    return !AppState::isPlaying && gbe::UndoRedoManager::GetInstance().CanRedo();
}

CameraComponent* HierarchyManager::GetMainCamera() const {
    return gbe::IInstanceManager<GameCamera>::getOldest();
}

HierarchyObject::Ref HierarchyManager::AddRootObject(std::unique_ptr<HierarchyObject> rootObj) {
    if (!rootObj) return nullptr;

    rootObj->SetParent(nullptr);

    m_rootNodes.push_back(std::move(rootObj));
    auto newref = m_rootNodes.back()->getRef();

    // Colliders may have been built mid-deserialization, before transforms/parents were final.
    if (Transform* t = m_rootNodes.back()->GetTransform()) {
        t->SyncPhysics();
    }

    gbe::EventSystem::DispatchTo(EVENT_ONOBJECTCREATED,std::make_unique<HierarchyObjectArgs>(newref));

    return newref;
}

std::unique_ptr<HierarchyObject> HierarchyManager::RemoveRootObject(HierarchyObject::Ref rootToRemove) {
    for (auto it = m_rootNodes.begin(); it != m_rootNodes.end(); ++it) {
        if (it->get() == rootToRemove.GetPtr()) {
            std::unique_ptr<HierarchyObject> detachedRoot = std::move(*it);
            detachedRoot->SetParent(nullptr);
            m_rootNodes.erase(it);
            return detachedRoot;
        }
    }
    return nullptr;
}

void HierarchyManager::QueueObjectDeletion(HierarchyObject::Ref object) {
    if (!object) return;

    // A submesh node goes with its object, never on its own: it would only be made again
    if (IsLiveSubMeshNode(object.GetPtr())) return;

    const auto objectId = object.GetID();
    for (const auto queuedId : m_deferredDeletionIds) {
        if (queuedId == objectId) return;
    }

    m_deferredDeletionIds.push_back(objectId);
}

bool HierarchyManager::ReparentObject(HierarchyObject::Ref object, HierarchyObject::Ref parent) {
    // A submesh node belongs to its object: it stays where it is (a node that stands for no submesh any more can be moved)
    if (HierarchyObject* node = object.GetPtr(); IsLiveSubMeshNode(node)) {
        return node->GetParent() == parent;
    }

    UndoScope undoScope;

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
        auto& siblings = oldParent.GetPtr()->MutableChildren();
        for (auto it = siblings.begin(); it != siblings.end(); ++it) {
            if (it->get() == objectPtr) {
                detachedObject = std::move(*it);
                siblings.erase(it);
                break;
            }
        }
    }
    else {
        detachedObject = RemoveRootObject(object);
    }

    if (!detachedObject) return false;

    detachedObject->SetParent(parentPtr);
    if (parentPtr) {
        parentPtr->MutableChildren().push_back(std::move(detachedObject));
    }
    else {
        m_rootNodes.push_back(std::move(detachedObject));
    }

    // The world transform changed with the new parent, so bodies must follow.
    if (Transform* t = objectPtr->GetTransform()) {
        t->SyncPhysics();
    }
    return true;
}

bool HierarchyManager::CopyObject(HierarchyObject::Ref object) {
    return CopyObjects({ object });
}

bool HierarchyManager::CopyObjects(const std::vector<HierarchyObject::Ref>& objects) {
    std::vector<gbe::SerializedData> clipboard;
    for (const auto& object : objects) {
        HierarchyObject* objectPtr = object.GetPtr();
        if (!objectPtr) continue;

        // A submesh node is copied with its object, not on its own: a copy elsewhere would stand for nothing
        if (IsLiveSubMeshNode(objectPtr)) continue;

        gbe::SerializedData data = objectPtr->Serialize();
        for (auto it = data.serialized_variables.begin();
             it != data.serialized_variables.end();) {
            const std::string& key = it->first;
            if (key == "m_guid" ||
                (key.size() > 7 && key.compare(key.size() - 7, 7, ".m_guid") == 0)) {
                it = data.serialized_variables.erase(it);
            }
            else {
                ++it;
            }
        }
        data.label = "HierarchyObjectClipboard";
        clipboard.push_back(std::move(data));
    }

    if (clipboard.empty()) return false;

    m_copiedObjects = std::move(clipboard);
    m_hasCopiedObject = true;
    return true;
}

bool HierarchyManager::CopyComponent(ComponentBase* component) {
    if (!component) return false;

    m_copiedComponent = component->Serialize();
    for (auto it = m_copiedComponent.serialized_variables.begin();
         it != m_copiedComponent.serialized_variables.end();) {
        const std::string& key = it->first;
        if (key == "m_guid") {
            it = m_copiedComponent.serialized_variables.erase(it);
        }
        else {
            ++it;
        }
    }
    m_copiedComponentType = typeid(*component).name();
    m_copiedComponentName = component->GetName();
    m_hasCopiedComponent = true;
    return true;
}

bool HierarchyManager::PasteComponent(HierarchyObject::Ref object) {
    if (!m_hasCopiedComponent || !object) return false;

    gbe::SerializedData componentData = m_copiedComponent;
    gbe::ISerializable* rawComponent =
        gbe::TypeRegistry::Instantiate(m_copiedComponentType, componentData);
    auto* component = dynamic_cast<ComponentBase*>(rawComponent);
    if (!component) {
        delete rawComponent;
        std::cerr << "Unable to instantiate copied component type: "
                  << m_copiedComponentType << std::endl;
        return false;
    }

    component->Deserialize(componentData);
    UndoScope undoScope;
    object.GetPtr()->AddComponent(std::unique_ptr<ComponentBase>(component));
    return true;
}

HierarchyObject::Ref HierarchyManager::PasteObject(HierarchyObject::Ref parent) {
    const auto pasted = PasteObjects(parent);
    return pasted.empty() ? HierarchyObject::Ref(nullptr) : pasted.front();
}

std::vector<HierarchyObject::Ref> HierarchyManager::PasteObjects(HierarchyObject::Ref parent) {
    std::vector<HierarchyObject::Ref> results;
    if (!m_hasCopiedObject) return results;

    UndoScope undoScope;

    for (const auto& copied : m_copiedObjects) {
        gbe::SerializedData pasteData = copied;
        gbe::ISerializable* rawObject = gbe::TypeRegistry::Instantiate(
            typeid(HierarchyObject).name(), pasteData);
        auto* pastedObject = dynamic_cast<HierarchyObject*>(rawObject);
        if (!pastedObject) {
            delete rawObject;
            continue;
        }

        pastedObject->Deserialize(pasteData);

        std::unique_ptr<HierarchyObject> ownedObject(pastedObject);
        HierarchyObject::Ref pastedRef = AddRootObject(std::move(ownedObject));
        if (!pastedRef) continue;

        //If parent assigned, but reparenting fails, do not continue pasting.
        if (parent && !ReparentObject(pastedRef, parent)) {
            RemoveRootObject(pastedRef);
            std::cerr << "Pasting to an invalid parent." << std::endl;
            return results;
        }

        if (pastedRef.GetPtr()->IsPrefabInstance()) {
            PrefabFeature::RefreshPrefabInstance(pastedRef);
        }

        results.push_back(pastedRef);
    }

    return results;
}

size_t HierarchyManager::CommitDeferredDeletions() {
    if (m_deferredDeletionIds.empty()) return 0;

    UndoScope undoScope;

    std::vector<gbe::IInstanceManager<HierarchyObject>::IdType> pendingDeletions;
    pendingDeletions.swap(m_deferredDeletionIds);

    size_t removedCount = 0;
    for (const auto objectId : pendingDeletions) {
        HierarchyObject::Ref object(objectId);
        HierarchyObject* objectPtr = object.GetPtr();
        if (!objectPtr) continue;

        gbe::EventSystem::DispatchTo(EVENT_ONOBJECTDESTROYED,std::make_unique<HierarchyObjectArgs>(object));

        if (objectPtr->GetParent() && !ReparentObject(objectPtr, nullptr)) {
            continue;
        }

        if (RemoveRootObject(object)) {
            ++removedCount;
        }
    }

    return removedCount;
}

HierarchyObject::Ref HierarchyManager::CreateNewEmpty(std::string name)
{
    UndoScope undoScope;

    if(name.size() == 0 || name.empty())
        name = "New Object";

    auto root = std::make_unique<HierarchyObject>(name);
    return HierarchyManager::GetInstance().AddRootObject(std::move(root));
}

void HierarchyManager::AddComponentToObject(HierarchyObject::Ref object, std::unique_ptr<ComponentBase> component) {
    if (!object || !component) return;

    UndoScope undoScope;
    //Moved it to object
    object.GetPtr()->AddComponent(std::move(component));
}

std::unique_ptr<ComponentBase> HierarchyManager::RemoveComponentFromObject(HierarchyObject::Ref object, ComponentBase* componentToRemove) {
    if (!object || !componentToRemove) return nullptr;

    UndoScope undoScope;
    //Moved it to object
    return object.GetPtr()->RemoveComponent(componentToRemove);
}

bool HierarchyManager::GetMainCameraMatrices(glm::mat4& outViewMatrix, glm::mat4& outProjectionMatrix) {
    CameraComponent* activeCamera = GetMainCamera();
    if (!activeCamera) {
        activeCamera = GetEditorCamera();
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

static void GatherModelsRecursive(HierarchyObject::Ref obj, const glm::mat4& parentMatrix, std::vector<ModelRenderInstance>& outModels, const ProjectMaterialSettings& materials) {
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
            instance.Overrides = modelComp->GetOverrides();

            // A submesh with a node of its own (SubMeshComponent) can override material properties too: its overrides are
            // laid over the object's. (A model with one submesh has no nodes: one that is left stands for nothing.)
            for (const auto& child : obj.GetPtr()->GetChildren()) {
                const SubMeshComponent* part = pModel->SubMeshes.size() > 1 && child ? child->GetComponent<SubMeshComponent>() : nullptr;
                if (!part || !part->HasAnyOverride()) continue;

                const int index = part->GetSubMeshIndex();
                if (index < 0 || static_cast<size_t>(index) >= pModel->SubMeshes.size()) continue;

                if (instance.SubMeshOverrides.empty()) {
                    instance.SubMeshOverrides.assign(pModel->SubMeshes.size(), instance.Overrides);
                }
                instance.SubMeshOverrides[static_cast<size_t>(index)].Apply(part->GetOverrides());
            }
            instance.AnyOverrideAlpha = instance.Overrides.OverrideAlpha;
            instance.AnyOverrideTransmission = instance.Overrides.OverrideTransmission;
            for (const MaterialOverrides& submeshOverrides : instance.SubMeshOverrides) {
                instance.AnyOverrideAlpha = instance.AnyOverrideAlpha || submeshOverrides.OverrideAlpha;
                instance.AnyOverrideTransmission = instance.AnyOverrideTransmission || submeshOverrides.OverrideTransmission;
            }

            // With the master switch off, none of the layers is there, whatever the switches below say
            instance.ClearcoatEnabled = materials.enabled && materials.clearcoat;
            instance.SheenEnabled = materials.enabled && materials.sheen;
            instance.TransmissionEnabled = materials.enabled && materials.transmission;
            instance.SpecularAAStrength = materials.enabled && materials.specular_aa ? glm::max(materials.specular_aa_strength, 0.0f) : 0.0f;

            // ADDED: Sort submeshes based on material transparency. A material that lets light through is kept apart:
            // the renderers that draw glass draw it with the glass, and the others as the solid it was before.
            for (uint32_t i = 0; i < pModel->SubMeshes.size(); ++i) {
                uint32_t matIndex = pModel->SubMeshes[i].MaterialIndex;
                const MaterialOverrides& over = instance.OverridesFor(i);
                const bool transmissive = instance.TransmissionEnabled
                    && (over.OverrideTransmission || pModel->PBRMaterials[matIndex].Transmission > 0.0f);
                if (over.OverrideAlpha || pModel->PBRMaterials[matIndex].IsTransparent) {
                    instance.TransparentSubmeshIndices.push_back(i);
                }
                else if (transmissive) {
                    instance.TransmissiveSubmeshIndices.push_back(i);
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
        GatherModelsRecursive(child.get(), currentWorldMatrix, outModels, materials);
    }
}

void HierarchyManager::GatherRenderModels(std::vector<ModelRenderInstance>& outModels, const ProjectMaterialSettings& materials) const
{
    // Clear any leftover data from the previous frame
    outModels.clear();

    // Start with an identity matrix for root-level objects
    glm::mat4 rootMatrix(1.0f);

    // Iterate through all root nodes managed by HierarchyManager
    for (const auto& rootNode : m_rootNodes) {
        GatherModelsRecursive(rootNode.get(), rootMatrix, outModels, materials);
    }
}

namespace {
    // What GatherLightsRecursive needs from ProjectPhysicalLightSettings
    struct LightGatherSettings {
        bool realUnits = false;
        float exposure = 1.0f;      // Scene value of one lux (or cd/m2) at the project's exposure
        bool lightSize = false;
    };

    // The sizes every light had before lights had one of their own
    constexpr float kFixedPointLightRadius = 0.1f;      // World units
    constexpr float kFixedSunAngularRadius = 0.01f;     // Radians

    glm::vec3 SafeNormalize(const glm::vec3& v, const glm::vec3& fallback) {
        const float lengthSquared = glm::dot(v, v);
        return lengthSquared > 1e-12f ? v * glm::inversesqrt(lengthSquared) : fallback;
    }

    // The slot a point, spot or rectangular light fills in: everything they share, at the light's position
    Diligent::PointLightData& NextLocalLight(Diligent::LightConstants& lights, const glm::mat4& world, float radius, const glm::vec3& color, float intensity, float range) {
        Diligent::PointLightData& slot = lights.PointLights[lights.NumPointLights++];
        slot = Diligent::PointLightData{};
        slot.Position = glm::vec4(glm::vec3(world[3]), radius);
        slot.Color = glm::vec4(color, intensity);
        slot.Range = range;
        return slot;
    }
}

static void GatherLightsRecursive(HierarchyObject::Ref obj, const glm::mat4& parentMatrix, Diligent::LightConstants& outLights, const LightGatherSettings& settings) {
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
            const float angularRadius = settings.lightSize ? glm::radians(glm::max(dirLight->GetAngularRadius(), 0.0f)) : kFixedSunAngularRadius;
            lightData.Direction = glm::vec4(worldDir, angularRadius);

            // Pack color and intensity using Getters from LightBase. Real units: lux, seen at the project's exposure.
            const float intensity = settings.realUnits ? dirLight->GetLux() * settings.exposure : dirLight->GetIntensity();
            lightData.Color = glm::vec4(dirLight->GetColor(), intensity);

            outLights.NumDirLights++;
        }
    }

    // Point, spot and rectangular lights share the point light slots.
    // Real units: a point or spot light's lumens become candela (all around), seen at the project's exposure.
    if (PointLight* pointLight = obj.GetPtr()->GetComponent<PointLight>()) {
        if (outLights.NumPointLights < Diligent::MAX_POINT_LIGHTS) {
            const float radius = settings.lightSize ? glm::max(pointLight->GetRadius(), 0.0f) : kFixedPointLightRadius;
            const float intensity = settings.realUnits ? LightUnits::LumensToCandela(pointLight->GetLumens()) * settings.exposure : pointLight->GetIntensity();
            NextLocalLight(outLights, currentWorldMatrix, radius, pointLight->GetColor(), intensity, pointLight->GetRange());
        }
    }

    if (SpotLight* spotLight = obj.GetPtr()->GetComponent<SpotLight>()) {
        if (outLights.NumPointLights < Diligent::MAX_POINT_LIGHTS) {
            const float radius = settings.lightSize ? glm::max(spotLight->GetRadius(), 0.0f) : kFixedPointLightRadius;
            const float intensity = settings.realUnits ? LightUnits::LumensToCandela(spotLight->GetLumens()) * settings.exposure : spotLight->GetIntensity();
            Diligent::PointLightData& lightData = NextLocalLight(outLights, currentWorldMatrix, radius, spotLight->GetColor(), intensity, spotLight->GetRange());

            // The cone points along the object's -Y axis, like a directional light
            lightData.SpotDirection = glm::vec4(SafeNormalize(glm::vec3(currentWorldMatrix * glm::vec4(0.0f, -1.0f, 0.0f, 0.0f)), glm::vec3(0.0f, -1.0f, 0.0f)), 0.0f);

            const float outerAngle = glm::clamp(spotLight->GetOuterAngle(), 1.0f, 89.0f);
            const float innerAngle = glm::clamp(spotLight->GetInnerAngle(), 0.0f, outerAngle);
            const float cosOuter = glm::cos(glm::radians(outerAngle));
            const float cosInner = glm::cos(glm::radians(innerAngle));
            lightData.SpotScale = 1.0f / glm::max(cosInner - cosOuter, 0.0001f);
            lightData.SpotOffset = -cosOuter * lightData.SpotScale;
        }
    }

    // A rectangular light turns lumens into the radiance of its panel: all of them leave through its front
    if (AreaLight* areaLight = obj.GetPtr()->GetComponent<AreaLight>()) {
        if (outLights.NumPointLights < Diligent::MAX_POINT_LIGHTS) {
            const float width = glm::max(areaLight->GetWidth(), 0.001f);
            const float height = glm::max(areaLight->GetHeight(), 0.001f);
            const float radiance = settings.realUnits ? LightUnits::LumensToRadiance(areaLight->GetLumens(), width * height) * settings.exposure : areaLight->GetIntensity();
            Diligent::PointLightData& lightData = NextLocalLight(outLights, currentWorldMatrix, 0.0f, areaLight->GetColor(), radiance, areaLight->GetRange());

            // The panel lies in the object's X and Z plane, so its width follows X and its height follows Z, whatever the object's scale
            const glm::vec3 axisX = SafeNormalize(glm::vec3(currentWorldMatrix[0]), glm::vec3(1.0f, 0.0f, 0.0f));
            const glm::vec3 axisZ = SafeNormalize(glm::vec3(currentWorldMatrix[2]), glm::vec3(0.0f, 0.0f, 1.0f));
            lightData.AxisX = glm::vec4(axisX * (width * 0.5f), 0.0f);
            lightData.AxisY = glm::vec4(axisZ * (height * 0.5f), 0.0f);
            lightData.SpotDirection = glm::vec4(SafeNormalize(glm::cross(axisX, axisZ), glm::vec3(0.0f, -1.0f, 0.0f)), 0.0f);
            lightData.RectArea = width * height;
        }
    }
    
    // Recursively process children
    for (const auto& child : obj.GetPtr()->GetChildren()) {
        GatherLightsRecursive(child.get(), currentWorldMatrix, outLights, settings);
    }
}

void HierarchyManager::GatherLightData(Diligent::LightConstants& outLights, const ProjectPhysicalLightSettings& physical) const {
    // Reset light counts for the current frame
    outLights.NumDirLights = 0;
    outLights.NumPointLights = 0;

    // With the master switch off every light is read as it was before lights had falloff, sizes and units of their own
    LightGatherSettings settings;
    settings.realUnits = physical.enabled && physical.real_units;
    settings.exposure = LightUnits::ExposureScale(physical.ev100);
    settings.lightSize = physical.enabled && physical.light_size;

    outLights.Flags = 0;
    if (physical.enabled && physical.inverse_square) outLights.Flags |= Diligent::LIGHT_FLAG_INVERSE_SQUARE;
    if (physical.enabled && physical.highlight_shape) outLights.Flags |= Diligent::LIGHT_FLAG_HIGHLIGHT_SHAPE;

    // Pass the camera position to the light constants for specular calculations
    glm::vec3 cameraPos;
    if (GetMainCameraPosition(cameraPos)) {
        outLights.CameraPos = glm::vec4(cameraPos, 1.0f);
    }

    glm::mat4 rootMatrix(1.0f);

    // Iterate through all root nodes managed by HierarchyManager
    for (const auto& rootNode : m_rootNodes) {
        GatherLightsRecursive(rootNode.get(), rootMatrix, outLights, settings);
    }
}

bool HierarchyManager::GetMainCameraPosition(glm::vec3& outPosition) const {
    CameraComponent* activeCamera = GetMainCamera();
    if (!activeCamera) {
        activeCamera = GetEditorCamera();
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
    PrefabFeature::SyncPrefabOverridesBeforeSave();
    return gbe::ISerializable::Serialize();
}

void HierarchyManager::Deserialize(gbe::SerializedData& data)
{
    this->m_sceneLabel = data.label;
    gbe::ISerializable::Deserialize(data);
    PrefabFeature::RefreshPrefabInstancesAfterLoad();
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
    //Call on unload BEFORE scene load commit
    gbe::EventSystem::DispatchTo(
        EVENT_ONSCENEUNLOAD,
        std::make_unique<SceneLoadArgs>(m_sceneLabel)
    );

    m_sceneFile = NormalizePath(filepath);
    this->DeserializeFromFile(m_sceneFile);
    gbe::UndoRedoManager::GetInstance().Clear();

    //Call on load AFTER scene load commit
    gbe::EventSystem::DispatchTo(
        EVENT_ONSCENELOAD,
        std::make_unique<SceneLoadArgs>(m_sceneLabel)
    );
}

void HierarchyManager::CreateNewScene()
{
    // Call on unload BEFORE replacing the current hierarchy.
    gbe::EventSystem::DispatchTo(
        EVENT_ONSCENEUNLOAD,
        std::make_unique<SceneLoadArgs>(m_sceneLabel)
    );

    m_rootNodes.clear();
    m_sceneFile.clear();
    m_sceneLabel = "Untitled";
    EnsureEditorCameraExists();
    gbe::UndoRedoManager::GetInstance().Clear();

    // Call on load AFTER the new blank scene is ready.
    gbe::EventSystem::DispatchTo(
        EVENT_ONSCENELOAD,
        std::make_unique<SceneLoadArgs>(m_sceneLabel)
    );
}

std::filesystem::path HierarchyManager::GetCurrentScene()
{
    return m_sceneFile;
}

void HierarchyManager::QuickSave()
{
    if(m_sceneFile.empty()){
        return;
    }
    
    this->SerializeToFile(m_sceneFile);
}
