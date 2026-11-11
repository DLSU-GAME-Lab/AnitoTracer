#pragma once

#include "ScriptHostAPI.hpp"

namespace ScriptHost::Bindings::field {
anito::FieldAPI GetAPI();
anito::ObjectId ResolveTransform(void*);
const char* ResolvePrefab(void*);
}
