#include "Bindings/Input.hpp"

#include "Input/InputMappedBool.hpp"
#include "Input/InputSystem.hpp"

#include <map>
#include <memory>
#include <string>

namespace ScriptHost::Bindings::input {

bool GetAction(const char* action) {
    static std::map<std::string, std::unique_ptr<gbe::InputMappedBool>> actions;
    if (!action) return false;
    auto it = actions.find(action);
    if (it == actions.end()) it = actions.emplace(action, std::make_unique<gbe::InputMappedBool>(action)).first;
    return it->second->Get();
}

void GetMouseDelta(anito::Vec3* out) {
    const auto delta = gbe::InputSystem::GetMouseDelta();
    *out = {delta.x, delta.y, 0.0f};
}

void GetMousePosition(anito::Vec3* out) {
    const auto position = gbe::InputSystem::GetMousePosition();
    *out = {position.x, position.y, 0.0f};
}

anito::InputAPI GetAPI() {
    return {GetAction, GetMouseDelta, GetMousePosition};
}
} // namespace ScriptHost::Bindings::input
