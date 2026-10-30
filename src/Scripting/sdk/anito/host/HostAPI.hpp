#pragma once

#include "Transform.hpp"
#include "Physics.hpp"
#include "Camera.hpp"
#include "Audio.hpp"
#include "Object.hpp"
#include "Scene.hpp"
#include "Script.hpp"
#include "Runtime.hpp"

namespace anito {
struct HostAPI {
    void (*Log)(const char* message);
    TransformAPI transform;
    PhysicsAPI physics;
    CameraAPI camera;
    AudioAPI audio;
    ObjectAPI object;
    SceneAPI scene;
    ScriptAPI script;
    InputAPI input;
    TimeAPI time;
    GuiAPI gui;
    FieldAPI field;
};

namespace detail {
// One pointer per loaded module; set by ScriptBehaviour's constructor before any script code runs.
inline const HostAPI*& HostPtr() {
    static const HostAPI* host = nullptr;
    return host;
}
inline const HostAPI& Host() { return *HostPtr(); }
} // namespace detail

} // namespace anito
