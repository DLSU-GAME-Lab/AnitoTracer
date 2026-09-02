#pragma once

#include "Camera.hpp"
#include "Types/OnGUI_Editor.hpp"

class EditorCamera : public CameraComponent, public gbe::IInstanceManager<EditorCamera>, public gbe::ITrigger<OnGUI_Editor> {
public:
    EditorCamera(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~EditorCamera() override = default;

    EditorCamera(const EditorCamera&) = delete;
    EditorCamera& operator=(const EditorCamera&) = delete;

    EditorCamera(EditorCamera&&) = default;
    EditorCamera& operator=(EditorCamera&&) = default;

    void OnGUI_EditorEvent(float deltaTime) override;

private:
    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(EditorCamera, CameraComponent);
};

GBE_REGISTER_SERIALIZED_TYPE(EditorCamera, ComponentBase);