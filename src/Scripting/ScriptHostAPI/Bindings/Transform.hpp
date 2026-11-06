#pragma once

#include "ScriptHostAPI.hpp"

namespace ScriptHost::Bindings::transform {
anito::TransformAPI GetAPI();
void GetPosition(anito::ObjectId, anito::Vec3*);
void SetPosition(anito::ObjectId, anito::Vec3);
void GetLocalPosition(anito::ObjectId, anito::Vec3*);
void SetLocalPosition(anito::ObjectId, anito::Vec3);
void GetRotation(anito::ObjectId, anito::Quat*);
void SetRotation(anito::ObjectId, anito::Quat);
void GetLocalRotation(anito::ObjectId, anito::Quat*);
void SetLocalRotation(anito::ObjectId, anito::Quat);
void GetLocalScale(anito::ObjectId, anito::Vec3*);
void SetLocalScale(anito::ObjectId, anito::Vec3);
void GetLossyScale(anito::ObjectId, anito::Vec3*);
void GetEulerAngles(anito::ObjectId, anito::Vec3*);
void SetEulerAngles(anito::ObjectId, anito::Vec3);
}
