#pragma once

#include "AnitoScriptSDK.hpp"

// Engine-side implementation of the script host API, grouped per category in ScriptHostAPI.cpp.
namespace ScriptHost {

const anito::HostAPI* GetAPI();

// Frame timing exposed through Time.deltaTime / Time.fixedDeltaTime.
void SetFrameDelta(float deltaTime);
void SetFixedDelta(float deltaTime);

} // namespace ScriptHost
