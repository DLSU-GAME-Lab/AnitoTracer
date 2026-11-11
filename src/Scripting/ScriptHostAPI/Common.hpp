#pragma once

#include "ScriptHostAPI.hpp"

#include "HierarchyObject.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <string>

namespace ScriptHost::Bindings::detail {

inline glm::vec3 ToGlm(anito::Vec3 value) {
    return {value.x, value.y, value.z};
}

inline anito::Vec3 FromGlm(const glm::vec3& value) {
    return {value.x, value.y, value.z};
}

inline glm::quat ToGlm(anito::Quat value) {
    return glm::quat(value.w, value.x, value.y, value.z);
}

inline anito::Quat FromGlm(const glm::quat& value) {
    return {value.x, value.y, value.z, value.w};
}

inline HierarchyObject* GetObject(anito::ObjectId id) {
    return id ? HierarchyObject::Ref(id).GetPtr() : nullptr;
}

template <typename Component>
Component* GetComponent(anito::ObjectId id) {
    HierarchyObject* object = GetObject(id);
    return object ? object->GetComponent<Component>() : nullptr;
}

inline anito::ObjectId GetId(const HierarchyObject* object) {
    return object ? object->getId() : 0;
}

inline const char* TransientString(const std::string& value) {
    thread_local std::string buffer;
    buffer = value;
    return buffer.c_str();
}

} // namespace ScriptHost::Bindings::detail
