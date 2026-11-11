#pragma once

#include "anito/generated/api/Transform.hpp"

namespace anito {
class Transform : public generated::TransformHandle {
public:
    using generated::TransformHandle::TransformHandle;

    bool valid() const { return generated::TransformHandle::valid(); }
    Transform parent() const;

    using generated::TransformHandle::eulerAngles;
    using generated::TransformHandle::localPosition;
    using generated::TransformHandle::localRotation;
    using generated::TransformHandle::localScale;
    using generated::TransformHandle::lossyScale;
    using generated::TransformHandle::position;
    using generated::TransformHandle::rotation;
    using generated::TransformHandle::setEulerAngles;
    using generated::TransformHandle::setLocalPosition;
    using generated::TransformHandle::setLocalRotation;
    using generated::TransformHandle::setLocalScale;
    using generated::TransformHandle::setPosition;
    using generated::TransformHandle::setRotation;

    Vec3 forward() const { return rotation() * Vec3{0.0f, 0.0f, 1.0f}; }
    Vec3 right() const { return rotation() * Vec3{1.0f, 0.0f, 0.0f}; }
    Vec3 up() const { return rotation() * Vec3{0.0f, 1.0f, 0.0f}; }

    // Rotates around a local-space axis.
    void rotate(Vec3 axis, float degrees) const { setLocalRotation(localRotation() * Quaternion::angleAxis(degrees, axis)); }
    // Moves in world space.
    void translate(Vec3 delta) const { setPosition(position() + delta); }
    void lookAt(Vec3 target, Vec3 worldUp) const { setRotation(Quaternion::lookRotation(target - position(), worldUp)); }
    Vec3 transformPoint(Vec3 local) const { return position() + rotation() * Vec3{local.x * lossyScale().x, local.y * lossyScale().y, local.z * lossyScale().z}; }
    Vec3 transformDirection(Vec3 local) const { return rotation() * local; }
    Vec3 inverseTransformDirection(Vec3 world) const { return rotation().inverse() * world; }

    bool operator==(const Transform& other) const { return id == other.id; }
    bool operator!=(const Transform& other) const { return id != other.id; }
};
} // namespace anito
