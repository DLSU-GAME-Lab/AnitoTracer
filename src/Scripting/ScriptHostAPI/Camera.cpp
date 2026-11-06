#include "Bindings/Camera.hpp"
#include "Common.hpp"

#include "Components/Camera.hpp"

namespace ScriptHost::Bindings::camera {
using detail::GetComponent;

bool HasCamera(anito::ObjectId id) {
    return GetComponent<CameraComponent>(id) != nullptr;
}

float GetFOV(anito::ObjectId id) {
    CameraComponent* camera = GetComponent<CameraComponent>(id);
    return camera ? camera->GetFOV() : 0.0f;
}

void SetFOV(anito::ObjectId id, float value) {
    if (CameraComponent* camera = GetComponent<CameraComponent>(id)) camera->SetFOV(value);
}

float GetAspect(anito::ObjectId id) {
    CameraComponent* camera = GetComponent<CameraComponent>(id);
    return camera ? camera->GetAspect() : 0.0f;
}

void SetAspect(anito::ObjectId id, float value) {
    if (CameraComponent* camera = GetComponent<CameraComponent>(id)) camera->SetAspect(value);
}

float GetNearPlane(anito::ObjectId id) {
    CameraComponent* camera = GetComponent<CameraComponent>(id);
    return camera ? camera->GetNearPlane() : 0.0f;
}

void SetNearPlane(anito::ObjectId id, float value) {
    if (CameraComponent* camera = GetComponent<CameraComponent>(id)) camera->SetNearPlane(value);
}

float GetFarPlane(anito::ObjectId id) {
    CameraComponent* camera = GetComponent<CameraComponent>(id);
    return camera ? camera->GetFarPlane() : 0.0f;
}

void SetFarPlane(anito::ObjectId id, float value) {
    if (CameraComponent* camera = GetComponent<CameraComponent>(id)) camera->SetFarPlane(value);
}

anito::CameraAPI GetAPI() {
    return {HasCamera, GetFOV, SetFOV, GetAspect, SetAspect, GetNearPlane, SetNearPlane, GetFarPlane, SetFarPlane};
}
} // namespace ScriptHost::Bindings::camera
