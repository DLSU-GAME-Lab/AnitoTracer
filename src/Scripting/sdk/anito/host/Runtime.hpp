#pragma once

#include "Common.hpp"

namespace anito {
// ---- Input / Time / Debug / Gui -----------------------------------------------------------------
struct InputAPI {
    bool (*GetAction)(const char* actionName);
    void (*GetMouseDelta)(Vec3*);
    void (*GetMousePosition)(Vec3*);
};

struct TimeAPI {
    float (*GetTime)();
    float (*GetDeltaTime)();
    float (*GetFixedDeltaTime)();
};

struct GuiAPI {
    // Screen position inside the game viewport; z < 0 means behind the camera.
    void (*WorldToScreen)(Vec3 world, Vec3* out);
    void (*FillRect)(Vec3 min, Vec3 max, Vec3 rgb, float alpha);
    void (*DrawRect)(Vec3 min, Vec3 max, Vec3 rgb, float alpha, float thickness);
};

// Resolves host-side exposed-field storage of reference types.
struct FieldAPI {
    ObjectId (*ResolveTransform)(void* storage);
    const char* (*ResolvePrefab)(void* storage);
};

} // namespace anito
