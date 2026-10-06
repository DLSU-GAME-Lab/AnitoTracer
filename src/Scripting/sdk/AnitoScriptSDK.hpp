#pragma once

// Boundary between the editor and compiled script modules. Must not include any engine header.

#include <cstdint>
#include <string>

#define ANITO_SCRIPT_ABI_VERSION 1

#if defined(_WIN32)
#define ANITO_SCRIPT_EXPORT extern "C" __declspec(dllexport)
#else
#define ANITO_SCRIPT_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace anito {

struct Vec3 { float x, y, z; };

inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator*(float s, Vec3 a) { return a * s; }
inline Vec3 operator/(Vec3 a, float s) { return {a.x / s, a.y / s, a.z / s}; }
inline Vec3& operator+=(Vec3& a, Vec3 b) { return a = a + b; }
inline Vec3& operator-=(Vec3& a, Vec3 b) { return a = a - b; }
inline Vec3& operator*=(Vec3& a, float s) { return a = a * s; }
inline Vec3& operator/=(Vec3& a, float s) { return a = a / s; }
inline bool operator==(Vec3 a, Vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
inline bool operator!=(Vec3 a, Vec3 b) { return !(a == b); }

// Same order as the host's ScriptFieldType.
enum class FieldType : int32_t { Float, Int, Bool, String, Vec3 };

// Engine services available to scripts; every call goes through the host so the DLL shares no engine statics.
struct HostAPI {
    void (*Log)(const char* message);
    void (*GetPosition)(void* owner, Vec3* out);
    void (*SetPosition)(void* owner, Vec3 value);
    void (*Rotate)(void* owner, Vec3 axis, float degrees);
};

struct ScriptContext {
    const HostAPI* host;
    void* owner;
    // Host-owned storage for exposed fields, one pointer per field in declaration order.
    void* const* fields;
};

class ScriptBehaviour {
public:
    explicit ScriptBehaviour(const ScriptContext& ctx) : m_ctx(ctx) {}
    virtual ~ScriptBehaviour() = default;

    virtual void OnStart() {}
    virtual void OnUpdate(float /*dt*/) {}
    virtual bool Invoke(const char* /*method*/) { return false; }

protected:
    void Log(const char* message) const { m_ctx.host->Log(message); }
    Vec3 GetPosition() const { Vec3 p{}; m_ctx.host->GetPosition(m_ctx.owner, &p); return p; }
    void SetPosition(Vec3 p) const { m_ctx.host->SetPosition(m_ctx.owner, p); }
    void Rotate(Vec3 axis, float degrees) const { m_ctx.host->Rotate(m_ctx.owner, axis, degrees); }

    ScriptContext m_ctx;
};

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
