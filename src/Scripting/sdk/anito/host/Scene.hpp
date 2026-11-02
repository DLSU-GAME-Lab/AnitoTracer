#pragma once

#include "Common.hpp"

namespace anito {
// ---- Scene / Camera -----------------------------------------------------------------------------
struct SceneAPI {
    ObjectId (*CreateEmpty)(const char* name, ObjectId parent);
    ObjectId (*CreatePrimitive)(const char* kind, ObjectId parent);
    ObjectId (*Instantiate)(const char* prefabPath, ObjectId parent);
    ObjectId (*Find)(const char* name);
    ObjectId (*FindWithScript)(const char* scriptName);
    ObjectId (*GetMainCamera)();
};

} // namespace anito
