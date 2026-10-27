#pragma once

// Unity-style handle classes over the host API. Every handle is a plain ObjectId wrapper: cheap to copy,
// safe to store across frames (a destroyed object just reports valid() == false and ignores calls).
//
// Categories: GameObject, Transform, Physics (Rigidbody/Collider/Physics), Audio (AudioSource),
// Scripts (Script), Scene (Scene/Camera), Input, Time, Gui, Debug.
// Property convention used by the script language: `x.foo` -> foo(), `x.foo = v` -> setFoo(v).

#include "Host.hpp"

#include <string>
#include <vector>

namespace anito {

class GameObject;
class Transform;
class Rigidbody;
class Collider;
class AudioSource;
class Script;

namespace detail {
inline const HostAPI& H() { return Host(); }
} // namespace detail

// ================================================================================================
// Transform
// ================================================================================================
class Transform {
public:
    ObjectId id = 0;

    bool valid() const { return detail::H().object.IsValid(id); }
    inline GameObject gameObject() const;
    inline Transform parent() const;

    // World space (Unity default); local* are relative to the parent.
    Vec3 position() const { Vec3 v{}; detail::H().transform.GetPosition(id, &v); return v; }
    void setPosition(Vec3 v) const { detail::H().transform.SetPosition(id, v); }
    Vec3 localPosition() const { Vec3 v{}; detail::H().transform.GetLocalPosition(id, &v); return v; }
    void setLocalPosition(Vec3 v) const { detail::H().transform.SetLocalPosition(id, v); }
    Quat rotation() const { Quat q; detail::H().transform.GetRotation(id, &q); return q; }
    void setRotation(Quat q) const { detail::H().transform.SetRotation(id, q); }
    Quat localRotation() const { Quat q; detail::H().transform.GetLocalRotation(id, &q); return q; }
    void setLocalRotation(Quat q) const { detail::H().transform.SetLocalRotation(id, q); }
    Vec3 localScale() const { Vec3 v{}; detail::H().transform.GetLocalScale(id, &v); return v; }
    void setLocalScale(Vec3 v) const { detail::H().transform.SetLocalScale(id, v); }
    Vec3 lossyScale() const { Vec3 v{}; detail::H().transform.GetLossyScale(id, &v); return v; }
    Vec3 eulerAngles() const { Vec3 v{}; detail::H().transform.GetEulerAngles(id, &v); return v; }
    void setEulerAngles(Vec3 v) const { detail::H().transform.SetEulerAngles(id, v); }

    Vec3 forward() const { return rotation() * Vec3{0.0f, 0.0f, 1.0f}; }
    Vec3 right() const { return rotation() * Vec3{1.0f, 0.0f, 0.0f}; }
    Vec3 up() const { return rotation() * Vec3{0.0f, 1.0f, 0.0f}; }

    // Rotates around a local-space axis.
    void rotate(Vec3 axis, float degrees) const { setLocalRotation(localRotation() * Quaternion::angleAxis(degrees, axis)); }
    // Moves in world space.
    void translate(Vec3 delta) const { setPosition(position() + delta); }
    void lookAt(Vec3 target, Vec3 worldUp) const { setRotation(Quaternion::lookRotation(target - position(), worldUp)); }
    Vec3 transformPoint(Vec3 local) const { return position() + rotation() * Vec3{local.x * lossyScale().x, local.y * lossyScale().y, local.z * lossyScale().z}; }
    Vec3 transformDirection(Vec3 local) const { return rotation() * local; }
    Vec3 inverseTransformDirection(Vec3 world) const { return rotation().inverse() * world; }

    bool operator==(const Transform& o) const { return id == o.id; }
    bool operator!=(const Transform& o) const { return id != o.id; }
};

// ================================================================================================
// Physics components
// ================================================================================================
class Rigidbody {
public:
    ObjectId id = 0;

    bool valid() const { return detail::H().physics.HasRigidbody(id); }
    inline GameObject gameObject() const;

    bool simulated() const { return detail::H().physics.IsSimulated(id); }
    Vec3 velocity() const { Vec3 v{}; detail::H().physics.GetVelocity(id, &v); return v; }
    void setVelocity(Vec3 v) const { detail::H().physics.SetVelocity(id, v); }
    Vec3 angularVelocity() const { Vec3 v{}; detail::H().physics.GetAngularVelocity(id, &v); return v; }
    void setAngularVelocity(Vec3 v) const { detail::H().physics.SetAngularVelocity(id, v); }
    float mass() const { return detail::H().physics.GetMass(id); }
    void setMass(float m) const { detail::H().physics.SetMass(id, m); }
    // Rotation of the simulated body, which can differ from the transform before the next sync.
    Quat rotation() const { Quat q; detail::H().physics.GetBodyRotation(id, &q); return q; }

    void addForce(Vec3 f) const { detail::H().physics.AddForce(id, f); }
    void addTorque(Vec3 t) const { detail::H().physics.AddTorque(id, t); }
    void addImpulse(Vec3 i) const { detail::H().physics.AddImpulse(id, i); }
    void teleport(Vec3 position, Quat rotation) const { detail::H().physics.Teleport(id, position, rotation); }
    void lockRotation(bool x, bool y, bool z) const { detail::H().physics.SetLockRotation(id, x, y, z); }

    bool operator==(const Rigidbody& o) const { return id == o.id; }
    bool operator!=(const Rigidbody& o) const { return id != o.id; }
};

class Collider {
public:
    ObjectId id = 0;

    bool valid() const { return detail::H().physics.HasCollider(id); }
    inline GameObject gameObject() const;

    bool operator==(const Collider& o) const { return id == o.id; }
    bool operator!=(const Collider& o) const { return id != o.id; }
};

class RaycastHit {
public:
    RaycastResult result{};

    bool hit() const { return result.hit != 0; }
    Vec3 point() const { return result.point; }
    inline GameObject gameObject() const;
};

struct Physics {
    static Vec3 gravity() { Vec3 g{}; detail::H().physics.GetGravity(&g); return g; }
    static void setGravity(Vec3 g) { detail::H().physics.SetGravity(g); }
    inline static RaycastHit raycast(Vec3 origin, Vec3 direction, float maxDistance, GameObject ignore);
};

// ================================================================================================
// Audio
// ================================================================================================
class AudioSource {
public:
    ObjectId id = 0;

    bool valid() const { return detail::H().audio.HasAudioSource(id); }
    inline GameObject gameObject() const;

    bool hasClip() const { return detail::H().audio.HasClip(id); }
    void play() const { detail::H().audio.Play(id); }
    void stop() const { detail::H().audio.Stop(id); }
    // Plays a fire-and-forget copy that keeps going after the owner is destroyed.
    void playDetached() const { detail::H().audio.PlayDetached(id); }
    float volume() const { return detail::H().audio.GetVolume(id); }
    void setVolume(float v) const { detail::H().audio.SetVolume(id, v); }
    bool loop() const { return detail::H().audio.GetLoop(id); }
    void setLoop(bool v) const { detail::H().audio.SetLoop(id, v); }
    float pitch() const { return detail::H().audio.GetPitch(id); }
    void setPitch(float v) const { detail::H().audio.SetPitch(id, v); }
    bool spatialized() const { return detail::H().audio.GetSpatialized(id); }
    void setSpatialized(bool v) const { detail::H().audio.SetSpatialized(id, v); }
    void setDistance(float minDistance, float maxDistance) const { detail::H().audio.SetDistance(id, minDistance, maxDistance); }

    bool operator==(const AudioSource& o) const { return id == o.id; }
    bool operator!=(const AudioSource& o) const { return id != o.id; }
};

// ================================================================================================
// Prefab asset reference (exposed field type)
// ================================================================================================
class PrefabAsset {
public:
    std::string path;
    bool empty() const { return path.empty(); }
};

inline PrefabAsset ResolvePrefabField(void* storage) {
    const char* p = detail::H().field.ResolvePrefab(storage);
    return PrefabAsset{p ? p : ""};
}

// ================================================================================================
// Script handle (cross-script access)
// ================================================================================================
namespace detail {
inline Variant ToVariant(float v) { return Variant::Of(v); }
inline Variant ToVariant(double v) { return Variant::Of(static_cast<float>(v)); }
inline Variant ToVariant(int v) { return Variant::Of(v); }
inline Variant ToVariant(bool v) { return Variant::Of(v); }
inline Variant ToVariant(Vec3 v) { return Variant::Of(v); }
inline Variant ToVariant(const std::string& v) { return Variant::Of(v.c_str()); }
inline Variant ToVariant(const char* v) { return Variant::Of(v); }
inline Variant ToVariant(const Variant& v) { return v; }
inline Variant ToVariant(const GameObject& v);
} // namespace detail

class Script {
public:
    ObjectId id = 0;
    std::string name;

    bool valid() const { return !name.empty() && detail::H().script.Has(id, name.c_str()); }
    inline GameObject gameObject() const;

    // Invokes a script method on the target; returns the Variant result (Kind::None when the call fails).
    template <class... Args>
    Variant callRaw(const std::string& method, const Args&... args) const {
        const Variant packed[sizeof...(Args) + 1] = {detail::ToVariant(args)..., Variant{}};
        Variant ret;
        detail::H().script.Call(id, name.c_str(), method.c_str(), packed, static_cast<uint32_t>(sizeof...(Args)), &ret);
        return ret;
    }
    template <class... Args> void call(const std::string& method, const Args&... args) const { callRaw(method, args...); }
    template <class... Args> float callFloat(const std::string& method, const Args&... args) const { return callRaw(method, args...).AsFloat(); }
    template <class... Args> int callInt(const std::string& method, const Args&... args) const { return callRaw(method, args...).AsInt(); }
    template <class... Args> bool callBool(const std::string& method, const Args&... args) const { return callRaw(method, args...).AsBool(); }
    template <class... Args> Vec3 callVec3(const std::string& method, const Args&... args) const { return callRaw(method, args...).AsVec3(); }
    template <class... Args> GameObject callObject(const std::string& method, const Args&... args) const;

    bool operator==(const Script& o) const { return id == o.id && name == o.name; }
    bool operator!=(const Script& o) const { return !(*this == o); }
};

// ================================================================================================
// GameObject
// ================================================================================================
class GameObject {
public:
    ObjectId id = 0;

    bool valid() const { return detail::H().object.IsValid(id); }
    std::string name() const { const char* n = detail::H().object.GetName(id); return n ? n : ""; }
    void setName(const std::string& n) const { detail::H().object.SetName(id, n.c_str()); }

    // Component getters.
    Transform transform() const { return Transform{id}; }
    Rigidbody rigidbody() const { return Rigidbody{id}; }
    Collider collider() const { return Collider{id}; }
    AudioSource audioSource() const { return AudioSource{id}; }
    Script getScript(const std::string& scriptName) const { return Script{id, scriptName}; }
    bool hasScript(const std::string& scriptName) const { return detail::H().script.Has(id, scriptName.c_str()); }
    Script addScript(const std::string& scriptName) const { detail::H().script.Add(id, scriptName.c_str()); return Script{id, scriptName}; }
    int componentCount() const { return detail::H().object.GetComponentCount(id); }
    std::string componentName(int index) const { const char* n = detail::H().object.GetComponentName(id, index); return n ? n : ""; }
    bool hasComponent(const std::string& typeName) const { return detail::H().object.HasComponent(id, typeName.c_str()); }

    Rigidbody addRigidbody(float mass) const { detail::H().object.AddRigidbody(id, mass); return Rigidbody{id}; }
    Collider addBoxCollider(Vec3 halfExtents) const { detail::H().object.AddCollider(id, 0, halfExtents); return Collider{id}; }
    Collider addSphereCollider(float radius) const { detail::H().object.AddCollider(id, 1, Vec3{radius, 0.0f, 0.0f}); return Collider{id}; }
    Collider addCapsuleCollider(float radius, float halfHeight) const { detail::H().object.AddCollider(id, 2, Vec3{radius, halfHeight, 0.0f}); return Collider{id}; }

    // Hierarchy.
    GameObject parent() const { return GameObject{detail::H().object.GetParent(id)}; }
    GameObject root() const { return GameObject{detail::H().object.GetRoot(id)}; }
    void setParent(GameObject p) const { detail::H().object.SetParent(id, p.id); }
    int childCount() const { return detail::H().object.GetChildCount(id); }
    GameObject child(int index) const { return GameObject{detail::H().object.GetChild(id, index)}; }
    GameObject findChild(const std::string& childName) const { return GameObject{detail::H().object.FindChild(id, childName.c_str())}; }

    // Queues destruction at the end of the frame.
    void destroy() const { detail::H().object.Destroy(id); }

    // Calls `method` on every script of this object that defines it (like Unity's SendMessage).
    template <class... Args>
    void sendMessage(const std::string& method, const Args&... args) const {
        const Variant packed[sizeof...(Args) + 1] = {detail::ToVariant(args)..., Variant{}};
        detail::H().script.SendMessage(id, method.c_str(), packed, static_cast<uint32_t>(sizeof...(Args)));
    }

    bool operator==(const GameObject& o) const { return id == o.id; }
    bool operator!=(const GameObject& o) const { return id != o.id; }
};

namespace detail {
inline Variant ToVariant(const GameObject& v) { return Variant::OfObject(v.id); }
} // namespace detail

inline GameObject Transform::gameObject() const { return GameObject{id}; }
inline Transform Transform::parent() const { return Transform{detail::H().object.GetParent(id)}; }
inline GameObject Rigidbody::gameObject() const { return GameObject{id}; }
inline GameObject Collider::gameObject() const { return GameObject{id}; }
inline GameObject AudioSource::gameObject() const { return GameObject{id}; }
inline GameObject Script::gameObject() const { return GameObject{id}; }
inline GameObject RaycastHit::gameObject() const { return GameObject{result.object}; }

template <class... Args>
GameObject Script::callObject(const std::string& method, const Args&... args) const {
    return GameObject{callRaw(method, args...).AsObject()};
}

inline RaycastHit Physics::raycast(Vec3 origin, Vec3 direction, float maxDistance, GameObject ignore) {
    RaycastHit h;
    detail::H().physics.Raycast(origin, direction, maxDistance, ignore.id, &h.result);
    return h;
}

inline Transform ResolveTransformField(void* storage) { return Transform{detail::H().field.ResolveTransform(storage)}; }

// Ordered list of GameObject handles, for scripts that track spawned objects.
class GameObjectList {
public:
    std::vector<ObjectId> items;

    void add(GameObject g) { items.push_back(g.id); }
    int count() const { return static_cast<int>(items.size()); }
    void clear() { items.clear(); }
    GameObject get(int index) const {
        return index >= 0 && index < count() ? GameObject{items[static_cast<size_t>(index)]} : GameObject{};
    }
    void removeAt(int index) {
        if (index >= 0 && index < count()) items.erase(items.begin() + index);
    }
    // Drops handles whose objects no longer exist.
    void prune() {
        std::vector<ObjectId> alive;
        for (ObjectId i : items) if (detail::H().object.IsValid(i)) alive.push_back(i);
        items.swap(alive);
    }
};

// ================================================================================================
// Scene / Camera
// ================================================================================================
struct Scene {
    static GameObject none() { return GameObject{}; }
    static GameObject createEmpty(const std::string& name, GameObject parent) { return GameObject{detail::H().scene.CreateEmpty(name.c_str(), parent.id)}; }
    // kind: "sphere" or "cube".
    static GameObject createPrimitive(const std::string& kind, GameObject parent) { return GameObject{detail::H().scene.CreatePrimitive(kind.c_str(), parent.id)}; }
    static GameObject instantiate(const PrefabAsset& prefab, GameObject parent) {
        return GameObject{prefab.empty() ? 0 : detail::H().scene.Instantiate(prefab.path.c_str(), parent.id)};
    }
    static GameObject find(const std::string& name) { return GameObject{detail::H().scene.Find(name.c_str())}; }
    static GameObject findWithScript(const std::string& scriptName) { return GameObject{detail::H().scene.FindWithScript(scriptName.c_str())}; }
};

struct Camera {
    static GameObject main() { return GameObject{detail::H().scene.GetMainCamera()}; }
};

// ================================================================================================
// Input / Time / Gui / Debug
// ================================================================================================
struct Input {
    // True while the named input action (e.g. "INPUT_KEY_THRUST_FORWARD") is held.
    static bool getAction(const std::string& action) { return detail::H().input.GetAction(action.c_str()); }
    static Vec3 mouseDelta() { Vec3 v{}; detail::H().input.GetMouseDelta(&v); return v; }
    static Vec3 mousePosition() { Vec3 v{}; detail::H().input.GetMousePosition(&v); return v; }
};

struct Time {
    static float time() { return detail::H().time.GetTime(); }
    static float deltaTime() { return detail::H().time.GetDeltaTime(); }
    static float fixedDeltaTime() { return detail::H().time.GetFixedDeltaTime(); }
};

// Immediate-mode drawing; only valid inside `on GUI`. Coordinates are screen pixels in the game view.
struct Gui {
    // z < 0 means the point is behind the camera.
    static Vec3 worldToScreen(Vec3 world) { Vec3 v{}; detail::H().gui.WorldToScreen(world, &v); return v; }
    static void fillRect(Vec3 min, Vec3 max, Vec3 rgb, float alpha) { detail::H().gui.FillRect(min, max, rgb, alpha); }
    static void drawRect(Vec3 min, Vec3 max, Vec3 rgb, float alpha, float thickness) { detail::H().gui.DrawRect(min, max, rgb, alpha, thickness); }
};

struct Debug {
    static void log(const std::string& message) { detail::H().Log(message.c_str()); }
};

// ================================================================================================
// ScriptBehaviour
// ================================================================================================
class ScriptBehaviour {
public:
    explicit ScriptBehaviour(const ScriptContext& ctx) : m_ctx(ctx), gameObject{ctx.object} { detail::HostPtr() = ctx.host; }
    virtual ~ScriptBehaviour() = default;

    virtual void OnStart() {}
    virtual void OnUpdate(float /*dt*/) {}
    virtual void OnFixedUpdate(float /*dt*/) {}
    virtual void OnGUI(float /*dt*/) {}
    virtual void OnCollisionEnter(GameObject /*other*/) {}
    virtual bool Invoke(const char* /*method*/) { return false; }
    virtual bool Call(const char* /*method*/, const Variant* /*args*/, uint32_t /*argc*/, Variant* /*ret*/) { return false; }

protected:
    ScriptContext m_ctx;

public:
    GameObject gameObject;
};

} // namespace anito
