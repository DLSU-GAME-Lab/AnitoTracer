#pragma once

#include "Common.hpp"

namespace anito {
// ---- GameObject / Component general getters ------------------------------------------------------
struct ObjectAPI {
    bool (*IsValid)(ObjectId);
    const char* (*GetName)(ObjectId);
    void (*SetName)(ObjectId, const char*);
    ObjectId (*GetParent)(ObjectId);
    ObjectId (*GetRoot)(ObjectId);
    void (*SetParent)(ObjectId, ObjectId parent);
    int32_t (*GetChildCount)(ObjectId);
    ObjectId (*GetChild)(ObjectId, int32_t index);
    ObjectId (*FindChild)(ObjectId, const char* name);
    int32_t (*GetComponentCount)(ObjectId);
    const char* (*GetComponentName)(ObjectId, int32_t index);
    bool (*HasComponent)(ObjectId, const char* typeName);
    void (*AddRigidbody)(ObjectId, float mass);
    // shape: 0 box (params = half extents), 1 sphere (x = radius), 2 capsule (x = radius, y = half height)
    void (*AddCollider)(ObjectId, int32_t shape, Vec3 params);
    void (*Destroy)(ObjectId);
};

} // namespace anito
