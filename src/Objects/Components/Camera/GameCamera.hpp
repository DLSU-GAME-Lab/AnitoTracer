#pragma once

#include "Camera.hpp"
#include "Types/OnGUI_Editor.hpp"

class GameCamera : public CameraComponent, public gbe::IInstanceManager<GameCamera>, public gbe::ITrigger<OnGUI_Editor> {
public:
    GameCamera(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~GameCamera() override = default;

    GameCamera(const GameCamera&) = delete;
    GameCamera& operator=(const GameCamera&) = delete;

    GameCamera(GameCamera&&) = default;
    GameCamera& operator=(GameCamera&&) = default;

    void OnGUI_EditorEvent(float deltaTime) override;

private:
    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(GameCamera, CameraComponent);
};

GBE_REGISTER_SERIALIZED_TYPE(GameCamera, ComponentBase);