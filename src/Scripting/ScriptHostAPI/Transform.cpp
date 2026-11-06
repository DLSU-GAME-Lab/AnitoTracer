#include "Bindings/Transform.hpp"
#include "Common.hpp"

#include "Components/Transform.hpp"

namespace ScriptHost::Bindings::transform {
using detail::FromGlm;
using detail::GetComponent;
using detail::ToGlm;

void GetPosition(anito::ObjectId id, anito::Vec3* out) {
    if (Transform* transform = GetComponent<Transform>(id)) *out = FromGlm(transform->GetPosition());
}

void SetPosition(anito::ObjectId id, anito::Vec3 value) {
    if (Transform* transform = GetComponent<Transform>(id)) transform->SetWorldPosition(ToGlm(value));
}

void GetLocalPosition(anito::ObjectId id, anito::Vec3* out) {
    if (Transform* transform = GetComponent<Transform>(id)) *out = FromGlm(transform->GetLocalPosition());
}

void SetLocalPosition(anito::ObjectId id, anito::Vec3 value) {
    if (Transform* transform = GetComponent<Transform>(id)) transform->SetPosition(ToGlm(value));
}

void GetRotation(anito::ObjectId id, anito::Quat* out) {
    if (Transform* transform = GetComponent<Transform>(id)) *out = FromGlm(transform->GetRotation());
}

void SetRotation(anito::ObjectId id, anito::Quat value) {
    if (Transform* transform = GetComponent<Transform>(id)) transform->SetWorldRotation(ToGlm(value));
}

void GetLocalRotation(anito::ObjectId id, anito::Quat* out) {
    if (Transform* transform = GetComponent<Transform>(id)) *out = FromGlm(transform->GetLocalRotation());
}

void SetLocalRotation(anito::ObjectId id, anito::Quat value) {
    if (Transform* transform = GetComponent<Transform>(id)) transform->SetRotation(ToGlm(value));
}

void GetLocalScale(anito::ObjectId id, anito::Vec3* out) {
    if (Transform* transform = GetComponent<Transform>(id)) *out = FromGlm(transform->GetLocalScale());
}

void SetLocalScale(anito::ObjectId id, anito::Vec3 value) {
    if (Transform* transform = GetComponent<Transform>(id)) transform->SetScale(ToGlm(value));
}

void GetLossyScale(anito::ObjectId id, anito::Vec3* out) {
    if (Transform* transform = GetComponent<Transform>(id)) *out = FromGlm(transform->GetScale());
}

void GetEulerAngles(anito::ObjectId id, anito::Vec3* out) {
    if (Transform* transform = GetComponent<Transform>(id)) *out = FromGlm(transform->GetEulerAnglesDegrees());
}

void SetEulerAngles(anito::ObjectId id, anito::Vec3 value) {
    if (Transform* transform = GetComponent<Transform>(id)) transform->SetEulerAnglesDegrees(ToGlm(value));
}

anito::TransformAPI GetAPI() {
    return {GetPosition, SetPosition, GetLocalPosition, SetLocalPosition, GetRotation, SetRotation,
            GetLocalRotation, SetLocalRotation, GetLocalScale, SetLocalScale, GetLossyScale,
            GetEulerAngles, SetEulerAngles};
}
} // namespace ScriptHost::Bindings::transform
