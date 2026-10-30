#pragma once

#include "Common.hpp"

namespace anito {
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
} // namespace anito
