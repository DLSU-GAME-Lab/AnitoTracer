#pragma once

#include <vector>
#include <memory>
#include <string>

#include "HierarchyObject.hpp"
#include "Components/ComponentBase.hpp"
#include "Components/Transform.hpp"
#include "Components/Camera.hpp"
#include "Components/ModelComponent.hpp"
#include "Models/ModelStructs.hpp"

#include "../Rendering/RenderData.hpp"
#include "Components/Lights/DirectionLight.hpp"

#include ANITO_SERIALIZATION_INCLUDES
#include "Initializer/ObjectInitializer.hpp" //Needed for object creation setup
#include "Initializer/ComponentInitializer.hpp" //Needed for component creation setup

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

    // Adds an existing root object and takes ownership.
    HierarchyObject::Ref AddRootObject(std::unique_ptr<HierarchyObject> rootObj);

    // Removes a root object by its exact pointer address and transfers ownership back to the caller.
    std::unique_ptr<HierarchyObject> RemoveRootObject(HierarchyObject::Ref rootToRemove);

    // Retrieves all active root nodes in the hierarchy tree.
    const std::vector<std::unique_ptr<HierarchyObject>>& GetRootObjects() const {
        return m_rootNodes;
    }

    // Adds a component to a specific HierarchyObject.
    // Note: Requires HierarchyManager to be a friend class of HierarchyObject.
    void AddComponentToObject(HierarchyObject::Ref object, std::unique_ptr<ComponentBase> component);

    // Removes a component from a specific HierarchyObject and returns ownership.
    std::unique_ptr<ComponentBase> RemoveComponentFromObject(HierarchyObject::Ref object, ComponentBase* componentToRemove);

    // Retrieves the current main camera.
    CameraComponent* GetMainCamera() const { return gbe::IInstanceManager<CameraComponent>::getOldest(); }

    bool GetMainCameraMatrices(glm::mat4& outViewMatrix, glm::mat4& outProjectionMatrix);

    // Gathers all Models and their evaluated world transforms
    void GatherRenderModels(std::vector<ModelRenderInstance>& outModels) const;

    // Gathers all active lights in the hierarchy and processes their world transforms
    void GatherLightData(Diligent::LightConstants& outLights) const;

    bool GetMainCameraPosition(glm::vec3& outPosition) const;

private:
    HierarchyManager() = default;
    ~HierarchyManager() = default;

    std::vector<std::unique_ptr<HierarchyObject>> m_rootNodes;
    GBE_SERIALIZE_FIELD(m_rootNodes);
public:
    
};