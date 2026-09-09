#pragma once

#include "Components/Transform.hpp"
#include "AssignableEvent/MethodRegistry.hpp"
#include "Types/OnGUI_Editor.hpp"

#include <string>

class TeleportMainCamera : public ComponentBase, public gbe::ITrigger<OnGUI_Editor> {
public:
    TeleportMainCamera(Transform* transform = nullptr, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~TeleportMainCamera() override = default;

    TeleportMainCamera(const TeleportMainCamera&) = delete;
    TeleportMainCamera& operator=(const TeleportMainCamera&) = delete;

    TeleportMainCamera(TeleportMainCamera&&) = default;
    TeleportMainCamera& operator=(TeleportMainCamera&&) = default;

    // Public method registered to GBE MethodRegistry
    void DoTeleport();
    void OnGUI_EditorEvent(float deltaTime) override;

private:
    glm::vec3 m_targetpos = {0,0,0};
    GBE_SERIALIZE_FIELD_W_NAME(m_targetpos, "Target Position");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(TeleportMainCamera, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(TeleportMainCamera, ComponentBase);

GBE_REGISTER_METHOD(TeleportMainCamera, DoTeleport);