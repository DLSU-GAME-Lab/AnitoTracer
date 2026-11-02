#pragma once

#include "Common.hpp"

namespace anito {
struct CameraAPI {
    bool (*HasCamera)(ObjectId);
    float (*GetFOV)(ObjectId);
    void (*SetFOV)(ObjectId, float);
    float (*GetAspect)(ObjectId);
    void (*SetAspect)(ObjectId, float);
    float (*GetNearPlane)(ObjectId);
    void (*SetNearPlane)(ObjectId, float);
    float (*GetFarPlane)(ObjectId);
    void (*SetFarPlane)(ObjectId, float);
};
} // namespace anito
