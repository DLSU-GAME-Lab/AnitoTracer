#pragma once

#include "Components/Transform.hpp"
#include "AssignableEvent/MethodRegistry.hpp"

#include <string>

class SceneChanger : public ComponentBase {
public:
    SceneChanger(Transform* transform = nullptr, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~SceneChanger() override;

    SceneChanger(const SceneChanger&) = delete;
    SceneChanger& operator=(const SceneChanger&) = delete;

    SceneChanger(SceneChanger&&) = default;
    SceneChanger& operator=(SceneChanger&&) = default;

    // Public method registered to GBE MethodRegistry
    void ChangeScene();

    void SetTargetScene(const std::string& scene) { m_targetScene = scene; }
    const std::string& GetTargetScene() const { return m_targetScene; }

private:
    std::string m_targetScene = "NextScene";
    GBE_SERIALIZE_FIELD_W_NAME(m_targetScene, "Target Scene");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(SceneChanger, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(SceneChanger, ComponentBase);

// Registers ChangeScene so gbe::UnityEvent and MethodRegistry can locate and invoke it
GBE_REGISTER_METHOD(SceneChanger, ChangeScene);