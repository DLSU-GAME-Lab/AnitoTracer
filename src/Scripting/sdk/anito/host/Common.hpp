#pragma once

// The cross-DLL contract. Engine-independent: generated modules only see this and the other sdk headers.
// Every host function addresses engine objects by ObjectId (the HierarchyObject instance id, 0 = none).

#include "../Math.hpp"

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

} // namespace anito
