#pragma once

#include "ScriptHostAPI.hpp"

namespace ScriptHost::Bindings::input {
anito::InputAPI GetAPI();
bool GetAction(const char*);
void GetMouseDelta(anito::Vec3*);
void GetMousePosition(anito::Vec3*);
}
