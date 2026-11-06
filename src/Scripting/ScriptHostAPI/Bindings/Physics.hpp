#pragma once

#include "ScriptHostAPI.hpp"

namespace ScriptHost::Bindings::physics {
anito::PhysicsAPI GetAPI();
bool HasRigidbody(anito::ObjectId);
bool IsSimulated(anito::ObjectId);
void GetVelocity(anito::ObjectId, anito::Vec3*);
void SetVelocity(anito::ObjectId, anito::Vec3);
void GetAngularVelocity(anito::ObjectId, anito::Vec3*);
void SetAngularVelocity(anito::ObjectId, anito::Vec3);
void AddForce(anito::ObjectId, anito::Vec3);
void AddTorque(anito::ObjectId, anito::Vec3);
void AddImpulse(anito::ObjectId, anito::Vec3);
float GetMass(anito::ObjectId);
void SetMass(anito::ObjectId, float);
void GetBodyRotation(anito::ObjectId, anito::Quat*);
void Teleport(anito::ObjectId, anito::Vec3, anito::Quat);
void SetLockRotation(anito::ObjectId, bool, bool, bool);
bool HasCollider(anito::ObjectId);
void GetGravity(anito::ObjectId, anito::Vec3*);
void SetGravity(anito::ObjectId, anito::Vec3);
void Raycast(anito::Vec3, anito::Vec3, float, anito::ObjectId, anito::RaycastResult*);
}
