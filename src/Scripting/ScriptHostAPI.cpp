#include "ScriptHostAPI.hpp"

#include "ScriptHostAPI/Bindings.hpp"

#include <glm/vec3.hpp>

#include <iostream>

static_assert(sizeof(anito::Vec3) == sizeof(glm::vec3), "Vec3 must match glm::vec3 layout");

namespace {

const anito::HostAPI kHostAPI = {
#include "generated/ScriptHostAPI/HostInitializers.inc"
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
