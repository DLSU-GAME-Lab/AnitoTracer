#pragma once

#include "ScriptHostAPI.hpp"

namespace ScriptHost::Bindings::camera {
anito::CameraAPI GetAPI();
bool HasCamera(anito::ObjectId);
float GetFOV(anito::ObjectId);
void SetFOV(anito::ObjectId, float);
float GetAspect(anito::ObjectId);
void SetAspect(anito::ObjectId, float);
float GetNearPlane(anito::ObjectId);
void SetNearPlane(anito::ObjectId, float);
float GetFarPlane(anito::ObjectId);
void SetFarPlane(anito::ObjectId, float);
}
