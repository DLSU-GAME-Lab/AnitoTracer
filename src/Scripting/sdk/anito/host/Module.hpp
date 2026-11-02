#pragma once

#include "HostAPI.hpp"

namespace anito {
struct ScriptContext {
    const HostAPI* host;
    ObjectId object;
    // Host-owned storage for exposed fields, one pointer per field in descriptor declaration order.
    void* const* fields;
};

class ScriptBehaviour;

struct ScriptEntry {
    const char* name;
    ScriptBehaviour* (*create)(const ScriptContext&);
    void (*destroy)(ScriptBehaviour*);
    uint32_t fieldCount;
    const FieldType* fieldTypes;
};

struct ModuleInfo {
    uint32_t abiVersion;
    uint32_t entryCount;
    const ScriptEntry* entries;
};

} // namespace anito
