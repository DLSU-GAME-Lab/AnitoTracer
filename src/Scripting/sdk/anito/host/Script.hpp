#pragma once

#include "Common.hpp"

namespace anito {
// ---- Cross-script access --------------------------------------------------------------------------
struct ScriptAPI {
    bool (*Has)(ObjectId, const char* script);
    void (*Add)(ObjectId, const char* script);
    bool (*Call)(ObjectId, const char* script, const char* method, const Variant* args, uint32_t argc, Variant* ret);
    void (*SendMessage)(ObjectId, const char* method, const Variant* args, uint32_t argc);
};

} // namespace anito
