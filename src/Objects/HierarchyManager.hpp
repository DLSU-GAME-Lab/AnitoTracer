#pragma once

#include <vector>
#include <memory>
#include <string>
#include <filesystem>


#include "HierarchyObject.hpp"
#include "Components/ComponentBase.hpp"
#include "PropertyDrawers/componentbase_drawer.hpp" // should come after #include "Components/ComponentBase.hpp"


#include "Components/Transform.hpp"
#include "Components/Camera.hpp"
#include "Components/EditorCamera.hpp"
#include "Components/GameCamera.hpp"
#include "Components/ModelComponent.hpp"
#include "Models/ModelStructs.hpp"

#include "../Rendering/RenderData.hpp"
#include "Components/Lights/DirectionLight.hpp"
#include "Components/Lights/PointLight.hpp"

#include ANITO_SERIALIZATION_INCLUDES
#include ANITO_EVENT_INCLUDES

#include "Initializer/ObjectInitializer.hpp" //Needed for object creation setup
#include "Initializer/ComponentInitializer.hpp" //Needed for component creation setup

namespace HierarchyFeatures {
    class PrefabFeature;
}

class HierarchyManager : public gbe::ISerializable {
public:
    // Retrieves the singleton instance of the manager.
    static HierarchyManager& GetInstance() {
        static HierarchyManager instance;
        return instance;
    }

    // Delete copy/move constructors and assignment operators to enforce the singleton pattern.
    HierarchyManager(const HierarchyManager&) = delete;
    HierarchyManager& operator=(const HierarchyManager&) = delete;
    HierarchyManager(HierarchyManager&&) = delete;
    HierarchyManager& operator=(HierarchyManager&&) = delete;

    // Queues an object and its entire subtree for removal at the next commit.
    void QueueObjectDeletion(HierarchyObject::Ref object);

    // Commits all object deletions queued since the previous commit.
    // Returns the number of queued objects that were found and removed.
    size_t CommitDeferredDeletions();

    HierarchyObject::Ref CreateNewEmpty(std::string name = "");

    // Moves an existing object under a new parent, or to the root when parent is empty.
    bool ReparentObject(HierarchyObject::Ref object, HierarchyObject::Ref parent);

    // Copies an object and its serialized subtree into the in-memory clipboard.
    bool CopyObject(HierarchyObject::Ref object);

    // Pastes the clipboard as a child of parent, or as a root when parent is empty.
    HierarchyObject::Ref PasteObject(HierarchyObject::Ref parent = nullptr);

    bool HasCopiedObject() const { return m_hasCopiedObject; }

    // Retrieves all active root nodes in the hierarchy tree.
    const std::vector<std::unique_ptr<HierarchyObject>>& GetRootObjects() const {
        return m_rootNodes;
    }

    // Explicitly destroys all root nodes
    void Clear() {
		m_rootNodes.clear();
    }

    // Adds a component to a specific HierarchyObject.
    // Note: Requires HierarchyManager to be a friend class of HierarchyObject.
    void AddComponentToObject(HierarchyObject::Ref object, std::unique_ptr<ComponentBase> component);

    // Removes a component from a specific HierarchyObject and returns ownership.
    std::unique_ptr<ComponentBase> RemoveComponentFromObject(HierarchyObject::Ref object, ComponentBase* componentToRemove);

    // Retrieves the current active game camera.
    CameraComponent* GetMainCamera() const;

    // Retrieves the current editor camera for editor-only tooling.
    EditorCamera* GetEditorCamera() const { return gbe::IInstanceManager<EditorCamera>::getOldest(); }

    bool GetMainCameraMatrices(glm::mat4& outViewMatrix, glm::mat4& outProjectionMatrix);

    // Gathers all Models and their evaluated world transforms
    void GatherRenderModels(std::vector<ModelRenderInstance>& outModels) const;

    // Gathers all active lights in the hierarchy and processes their world transforms
    void GatherLightData(Diligent::LightConstants& outLights) const;

    bool GetMainCameraPosition(glm::vec3& outPosition) const;

    // ========================================================================
    // Event Dispatching Methods
    // ========================================================================

    /**
     * @brief Dispatches an event globally across ALL active root objects and their entire hierarchy subtrees.
     */
    template <typename TEvent>
    void DispatchEventData(const TEvent& event) {
        std::vector<HierarchyObject::Ref> rootsToDispatch;
        rootsToDispatch.reserve(m_rootNodes.size());
        for (const auto& root : m_rootNodes) {
            if (root) {
                rootsToDispatch.push_back(root->getRef());
            }
        }

        for (const HierarchyObject::Ref rootRef : rootsToDispatch) {
            if (HierarchyObject* root = rootRef.GetPtr()) {
                root->DispatchEventData(event, /*recursive=*/true);
            }
        }
    }

    /**
     * @brief Overload to construct a global event in-place.
     * Usage: HierarchyManager::GetInstance().DispatchGlobalEvent<UpdateEvent>(deltaTime);
     */
    template <typename TEvent, typename... Args>
    void DispatchEvent(Args&&... args) {
        TEvent event{ std::forward<Args>(args)... };
        DispatchEventData<TEvent>(event);
    }

    virtual gbe::SerializedData Serialize() override;
    virtual void Deserialize(gbe::SerializedData& data) override;

    // Ensures editor tooling always has a valid editor camera after scene loads.
    void EnsureEditorCameraExists();

    void LoadScene(std::filesystem::path filepath);
    std::filesystem::path GetCurrentScene();
    std::filesystem::path GetSceneFile() const { return m_sceneFile; }
    void QuickSave();

private:
    HierarchyManager() = default;
    ~HierarchyManager() = default;

    std::string m_sceneLabel = "";
    std::filesystem::path m_sceneFile = "";

    std::vector<std::unique_ptr<HierarchyObject>> m_rootNodes;
    GBE_SERIALIZE_FIELD(m_rootNodes);

    std::vector<gbe::IInstanceManager<HierarchyObject>::IdType> m_deferredDeletionIds;
    gbe::SerializedData m_copiedObject;
    bool m_hasCopiedObject = false;

    friend class PrefabFeature;

    // Internal Adder
    HierarchyObject::Ref AddRootObject(std::unique_ptr<HierarchyObject> rootObj);
    // Internal Remover
    std::unique_ptr<HierarchyObject> RemoveRootObject(HierarchyObject::Ref rootToRemove);

public:
    
};