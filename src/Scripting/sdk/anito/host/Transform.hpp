#pragma once

#include "Common.hpp"

namespace anito {
// ---- Transform ----------------------------------------------------------------------------------
struct TransformAPI {
    void (*GetPosition)(ObjectId, Vec3*);
    void (*SetPosition)(ObjectId, Vec3);
    void (*GetLocalPosition)(ObjectId, Vec3*);
    void (*SetLocalPosition)(ObjectId, Vec3);
    void (*GetRotation)(ObjectId, Quat*);
    void (*SetRotation)(ObjectId, Quat);
    void (*GetLocalRotation)(ObjectId, Quat*);
    void (*SetLocalRotation)(ObjectId, Quat);
    void (*GetLocalScale)(ObjectId, Vec3*);
    void (*SetLocalScale)(ObjectId, Vec3);
    void (*GetLossyScale)(ObjectId, Vec3*);
    void (*GetEulerAngles)(ObjectId, Vec3*);
    void (*SetEulerAngles)(ObjectId, Vec3);
};

} // namespace anito
