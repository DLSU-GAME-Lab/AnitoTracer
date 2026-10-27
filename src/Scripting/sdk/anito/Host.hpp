#pragma once

// The cross-DLL contract. Engine-independent: generated modules only see this and the other sdk headers.
// Every host function addresses engine objects by ObjectId (the HierarchyObject instance id, 0 = none).

#include "Math.hpp"

#include <cstdint>

#define ANITO_SCRIPT_ABI_VERSION 2

#if defined(_WIN32)
#define ANITO_SCRIPT_EXPORT extern "C" __declspec(dllexport)
#else
#define ANITO_SCRIPT_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace anito {

using ObjectId = uint64_t;

// Same order as the host's ScriptFieldType.
enum class FieldType : int32_t { Float, Int, Bool, String, Vec3, Prefab, Transform };

// Tagged value used to pass arguments between scripts (Script.call / GameObject.sendMessage).
struct Variant {
    enum class Kind : int32_t { None, Float, Int, Bool, Vec3, Object, String };

    Kind kind = Kind::None;
    float f = 0.0f;
    int32_t i = 0;
    bool b = false;
    Vec3 v{0.0f, 0.0f, 0.0f};
    ObjectId object = 0;
    const char* s = "";

    static Variant Of(float x) { Variant r; r.kind = Kind::Float; r.f = x; return r; }
    static Variant Of(int x) { Variant r; r.kind = Kind::Int; r.i = x; return r; }
    static Variant Of(bool x) { Variant r; r.kind = Kind::Bool; r.b = x; return r; }
    static Variant Of(Vec3 x) { Variant r; r.kind = Kind::Vec3; r.v = x; return r; }
    static Variant Of(const char* x) { Variant r; r.kind = Kind::String; r.s = x; return r; }
    static Variant OfObject(ObjectId x) { Variant r; r.kind = Kind::Object; r.object = x; return r; }

    float AsFloat() const { return kind == Kind::Int ? static_cast<float>(i) : f; }
    int AsInt() const { return kind == Kind::Float ? static_cast<int>(f) : i; }
    bool AsBool() const { return b; }
    Vec3 AsVec3() const { return v; }
    ObjectId AsObject() const { return object; }
    const char* AsString() const { return s; }
};

struct RaycastResult {
    uint32_t hit;
    ObjectId object;
    Vec3 point;
};

// ---- Transform ----------------------------------------------------------------------------------
struct TransformAPI {
    void (*GetPosition)(ObjectId, Vec3*);
    void (*SetPosition)(ObjectId, Vec3);
    void (*GetLocalPosition)(ObjectId, Vec3*);
    void (*SetLocalPosition)(ObjectId, Vec3);
    void (*GetRotation)(ObjectId, Quat*);
    void (*SetRotation)(ObjectId, Quat);
    void (*GetLocalRotation)(ObjectId, Quat*);
    void (*SetLocalRotation)(ObjectId, Quat);
    void (*GetLocalScale)(ObjectId, Vec3*);
    void (*SetLocalScale)(ObjectId, Vec3);
    void (*GetLossyScale)(ObjectId, Vec3*);
    void (*GetEulerAngles)(ObjectId, Vec3*);
    void (*SetEulerAngles)(ObjectId, Vec3);
};

// ---- Physics ------------------------------------------------------------------------------------
struct PhysicsAPI {
    bool (*HasRigidbody)(ObjectId);
    bool (*IsSimulated)(ObjectId);
    void (*GetVelocity)(ObjectId, Vec3*);
    void (*SetVelocity)(ObjectId, Vec3);
    void (*GetAngularVelocity)(ObjectId, Vec3*);
    void (*SetAngularVelocity)(ObjectId, Vec3);
    void (*AddForce)(ObjectId, Vec3);
    void (*AddTorque)(ObjectId, Vec3);
    void (*AddImpulse)(ObjectId, Vec3);
    float (*GetMass)(ObjectId);
    void (*SetMass)(ObjectId, float);
    void (*GetBodyRotation)(ObjectId, Quat*);
    void (*Teleport)(ObjectId, Vec3, Quat);
    void (*SetLockRotation)(ObjectId, bool x, bool y, bool z);
    bool (*HasCollider)(ObjectId);
    void (*GetGravity)(Vec3*);
    void (*SetGravity)(Vec3);
    void (*Raycast)(Vec3 origin, Vec3 direction, float maxDistance, ObjectId ignore, RaycastResult* out);
};

// ---- Audio --------------------------------------------------------------------------------------
struct AudioAPI {
    bool (*HasAudioSource)(ObjectId);
    bool (*HasClip)(ObjectId);
    void (*Play)(ObjectId);
    void (*Stop)(ObjectId);
    void (*PlayDetached)(ObjectId);
    float (*GetVolume)(ObjectId);
    void (*SetVolume)(ObjectId, float);
    bool (*GetLoop)(ObjectId);
    void (*SetLoop)(ObjectId, bool);
    float (*GetPitch)(ObjectId);
    void (*SetPitch)(ObjectId, float);
    bool (*GetSpatialized)(ObjectId);
    void (*SetSpatialized)(ObjectId, bool);
    void (*SetDistance)(ObjectId, float minDistance, float maxDistance);
};

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

// ---- Scene / Camera -----------------------------------------------------------------------------
struct SceneAPI {
    ObjectId (*CreateEmpty)(const char* name, ObjectId parent);
    ObjectId (*CreatePrimitive)(const char* kind, ObjectId parent);
    ObjectId (*Instantiate)(const char* prefabPath, ObjectId parent);
    ObjectId (*Find)(const char* name);
    ObjectId (*FindWithScript)(const char* scriptName);
    ObjectId (*GetMainCamera)();
};

// ---- Cross-script access --------------------------------------------------------------------------
struct ScriptAPI {
    bool (*Has)(ObjectId, const char* script);
    void (*Add)(ObjectId, const char* script);
    bool (*Call)(ObjectId, const char* script, const char* method, const Variant* args, uint32_t argc, Variant* ret);
    void (*SendMessage)(ObjectId, const char* method, const Variant* args, uint32_t argc);
};

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

struct HostAPI {
    void (*Log)(const char* message);
    TransformAPI transform;
    PhysicsAPI physics;
    AudioAPI audio;
    ObjectAPI object;
    SceneAPI scene;
    ScriptAPI script;
    InputAPI input;
    TimeAPI time;
    GuiAPI gui;
    FieldAPI field;
};

namespace detail {
// One pointer per loaded module; set by ScriptBehaviour's constructor before any script code runs.
inline const HostAPI*& HostPtr() {
    static const HostAPI* host = nullptr;
    return host;
}
inline const HostAPI& Host() { return *HostPtr(); }
} // namespace detail

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
