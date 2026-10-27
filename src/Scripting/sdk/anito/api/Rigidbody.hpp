#pragma once

#include "Common.hpp"

namespace anito {
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
} // namespace anito
