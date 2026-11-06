#include "ScriptHostAPI.hpp"

#include "ScriptHostAPI/Bindings.hpp"

#include <glm/vec3.hpp>

#include <iostream>

static_assert(sizeof(anito::Vec3) == sizeof(glm::vec3), "Vec3 must match glm::vec3 layout");

namespace {

const anito::HostAPI kHostAPI = {
    ScriptHost::Bindings::Log,
    ScriptHost::Bindings::transform::GetAPI(),
    ScriptHost::Bindings::physics::GetAPI(),
    ScriptHost::Bindings::camera::GetAPI(),
    ScriptHost::Bindings::audio::GetAPI(),
    ScriptHost::Bindings::object::GetAPI(),
    ScriptHost::Bindings::scene::GetAPI(),
    ScriptHost::Bindings::scripts::GetAPI(),
    ScriptHost::Bindings::input::GetAPI(),
    ScriptHost::Bindings::time::GetAPI(),
    ScriptHost::Bindings::gui::GetAPI(),
    ScriptHost::Bindings::field::GetAPI(),
};

} // namespace

namespace ScriptHost {

const anito::HostAPI* GetAPI() {
    return &kHostAPI;
}

void SetFrameDelta(float deltaTime) {
    Bindings::time::SetFrameDelta(deltaTime);
}

void SetFixedDelta(float deltaTime) {
    Bindings::time::SetFixedDelta(deltaTime);
}

} // namespace ScriptHost

namespace ScriptHost::Bindings {

void Log(const char* message) {
    std::cerr << "[Script] " << (message ? message : "") << std::endl;
}

} // namespace ScriptHost::Bindings
