#include "anito/host/Audio.hpp"
#include "anito/host/Camera.hpp"
#include "anito/host/Physics.hpp"
#include "anito/host/Transform.hpp"
#include "AnitoScriptSDK.hpp"

#include <cstddef>
#include <type_traits>

namespace {

struct PreviousHostAPI {
    void (*Log)(const char*);
    anito::TransformAPI transform;
    anito::PhysicsAPI physics;
    anito::CameraAPI camera;
    anito::AudioAPI audio;
    anito::ObjectAPI object;
    anito::SceneAPI scene;
    anito::ScriptAPI script;
    anito::InputAPI input;
    anito::TimeAPI time;
    anito::GuiAPI gui;
    anito::FieldAPI field;
};

static_assert(std::is_standard_layout_v<anito::HostAPI>);
static_assert(sizeof(anito::HostAPI) == sizeof(PreviousHostAPI));
static_assert(alignof(anito::HostAPI) == alignof(PreviousHostAPI));
#define CHECK_FIELD(field) \
    static_assert(offsetof(anito::HostAPI, field) == offsetof(PreviousHostAPI, field)); \
    static_assert(std::is_same_v<decltype(anito::HostAPI::field), decltype(PreviousHostAPI::field)>)
CHECK_FIELD(Log);
CHECK_FIELD(transform);
CHECK_FIELD(physics);
CHECK_FIELD(camera);
CHECK_FIELD(audio);
CHECK_FIELD(object);
CHECK_FIELD(scene);
CHECK_FIELD(script);
CHECK_FIELD(input);
CHECK_FIELD(time);
CHECK_FIELD(gui);
CHECK_FIELD(field);
#undef CHECK_FIELD

float GetVolume(anito::ObjectId id) { return id == 42 ? 0.75f : 0.0f; }
anito::ObjectId GetMainCamera() { return 73; }

} // namespace

int main() {
    anito::HostAPI host{};
    host.audio.AudioSourceGetVolume = GetVolume;
    host.scene.GetMainCamera = GetMainCamera;
    anito::detail::HostPtr() = &host;
    return anito::AudioSource{42}.volume() == 0.75f && anito::Scene::mainCamera().id == 73 ? 0 : 1;
}
