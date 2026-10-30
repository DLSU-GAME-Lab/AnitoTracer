#pragma once

#include "Common.hpp"

namespace anito {
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

} // namespace anito
