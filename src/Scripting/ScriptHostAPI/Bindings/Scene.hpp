#pragma once

#include "ScriptHostAPI.hpp"

namespace ScriptHost::Bindings::scene {
anito::SceneAPI GetAPI();
anito::ObjectId CreateEmpty(const char*, anito::ObjectId);
anito::ObjectId CreatePrimitive(const char*, anito::ObjectId);
anito::ObjectId Instantiate(const char*, anito::ObjectId);
anito::ObjectId Find(const char*);
anito::ObjectId FindWithScript(const char*);
anito::ObjectId GetMainCamera();
}
