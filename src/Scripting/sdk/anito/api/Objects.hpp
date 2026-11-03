#pragma once

#include "Components.hpp"

namespace anito {
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
    Camera camera() const { return Camera{id}; }
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
inline GameObject Camera::gameObject() const { return GameObject{id}; }
inline GameObject Rigidbody::gameObject() const { return GameObject{id}; }
inline GameObject Collider::gameObject() const { return GameObject{id}; }
inline GameObject AudioSource::gameObject() const { return GameObject{id}; }
inline GameObject Script::gameObject() const { return GameObject{id}; }
inline GameObject RaycastHit::gameObject() const { return GameObject{result.object}; }

template <class... Args>
GameObject Script::callObject(const std::string& method, const Args&... args) const {
    return GameObject{callRaw(method, args...).AsObject()};
}

inline Transform ResolveTransformField(void* storage) { return Transform{detail::H().field.ResolveTransform(storage)}; }

} // namespace anito
