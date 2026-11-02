#pragma once

#include "Common.hpp"

namespace anito {
class Camera {
public:
    ObjectId id = 0;

    bool valid() const { return detail::H().camera.HasCamera(id); }
    inline GameObject gameObject() const;

    float fov() const { return detail::H().camera.GetFOV(id); }
    void setFov(float value) const { detail::H().camera.SetFOV(id, value); }
    float aspect() const { return detail::H().camera.GetAspect(id); }
    void setAspect(float value) const { detail::H().camera.SetAspect(id, value); }
    float nearPlane() const { return detail::H().camera.GetNearPlane(id); }
    void setNearPlane(float value) const { detail::H().camera.SetNearPlane(id, value); }
    float farPlane() const { return detail::H().camera.GetFarPlane(id); }
    void setFarPlane(float value) const { detail::H().camera.SetFarPlane(id, value); }
};
} // namespace anito
