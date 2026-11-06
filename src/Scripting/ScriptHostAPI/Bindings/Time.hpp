#pragma once

#include "ScriptHostAPI.hpp"

namespace ScriptHost::Bindings::time {
anito::TimeAPI GetAPI();
float GetTime();
float GetDeltaTime();
float GetFixedDeltaTime();
void SetFrameDelta(float);
void SetFixedDelta(float);
}
