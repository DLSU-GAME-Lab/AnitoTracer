#include "Bindings/Physics.hpp"
#include "Common.hpp"

#include "Components/Physics/Collider.hpp"
#include "Components/Physics/RigidBody.hpp"

namespace ScriptHost::Bindings::physics {
using detail::FromGlm;
using detail::GetComponent;
using detail::GetObject;
using detail::ToGlm;

bool HasRigidbody(anito::ObjectId id) {
    return GetComponent<RigidBody>(id) != nullptr;
}

bool IsSimulated(anito::ObjectId id) {
    RigidBody* rigidBody = GetComponent<RigidBody>(id);
    return rigidBody && rigidBody->IsSimulated();
}

void GetVelocity(anito::ObjectId id, anito::Vec3* out) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) *out = FromGlm(rigidBody->GetVelocity());
}

void SetVelocity(anito::ObjectId id, anito::Vec3 value) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) rigidBody->SetVelocity(ToGlm(value));
}

void GetAngularVelocity(anito::ObjectId id, anito::Vec3* out) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) *out = FromGlm(rigidBody->GetAngularVelocity());
}

void SetAngularVelocity(anito::ObjectId id, anito::Vec3 value) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) rigidBody->SetAngularVelocity(ToGlm(value));
}

void AddForce(anito::ObjectId id, anito::Vec3 value) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) rigidBody->ApplyForce(ToGlm(value));
}

void AddTorque(anito::ObjectId id, anito::Vec3 value) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) rigidBody->ApplyTorque(ToGlm(value));
}

void AddImpulse(anito::ObjectId id, anito::Vec3 value) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) rigidBody->ApplyImpulse(ToGlm(value));
}

float GetMass(anito::ObjectId id) {
    RigidBody* rigidBody = GetComponent<RigidBody>(id);
    return rigidBody ? rigidBody->GetMass() : 0.0f;
}

void SetMass(anito::ObjectId id, float mass) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) rigidBody->SetMass(mass);
}

void GetBodyRotation(anito::ObjectId id, anito::Quat* out) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) *out = FromGlm(rigidBody->GetBodyRotation());
}

void Teleport(anito::ObjectId id, anito::Vec3 position, anito::Quat rotation) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) rigidBody->Teleport(ToGlm(position), ToGlm(rotation));
}

void SetLockRotation(anito::ObjectId id, bool x, bool y, bool z) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) rigidBody->SetLockRotation(x, y, z);
}

bool HasCollider(anito::ObjectId id) {
    return GetComponent<Collider>(id) != nullptr;
}

void GetGravity(anito::ObjectId id, anito::Vec3* out) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) *out = FromGlm(rigidBody->GetWorldGravity());
}

void SetGravity(anito::ObjectId id, anito::Vec3 value) {
    if (RigidBody* rigidBody = GetComponent<RigidBody>(id)) rigidBody->SetWorldGravity(ToGlm(value));
}

void Raycast(anito::Vec3 origin, anito::Vec3 direction, float maxDistance, anito::ObjectId ignore,
             anito::RaycastResult* out) {
    *out = {};
    RigidBody* rigidBody = GetComponent<RigidBody>(ignore);
    if (!rigidBody) return;

    const PhysicsRaycastHit hit = rigidBody->Raycast(ToGlm(origin), ToGlm(direction), maxDistance);
    out->hit = hit.hit ? 1 : 0;
    out->point = FromGlm(hit.point);
    out->object = hit.object;
}

anito::PhysicsAPI GetAPI() {
    return {HasRigidbody, IsSimulated, GetVelocity, SetVelocity, GetAngularVelocity, SetAngularVelocity,
            AddForce, AddTorque, AddImpulse, GetMass, SetMass, GetBodyRotation, Teleport, SetLockRotation,
            HasCollider, GetGravity, SetGravity, Raycast};
}
} // namespace ScriptHost::Bindings::physics
