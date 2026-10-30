#pragma once

#include "Common.hpp"

namespace anito {
// ---- Audio --------------------------------------------------------------------------------------
struct AudioAPI {
    bool (*HasAudioSource)(ObjectId);
    bool (*HasClip)(ObjectId);
    void (*Play)(ObjectId);
    void (*Stop)(ObjectId);
    void (*PlayDetached)(ObjectId);
    float (*GetVolume)(ObjectId);
    void (*SetVolume)(ObjectId, float);
    bool (*GetLoop)(ObjectId);
    void (*SetLoop)(ObjectId, bool);
    float (*GetPitch)(ObjectId);
    void (*SetPitch)(ObjectId, float);
    bool (*GetSpatialized)(ObjectId);
    void (*SetSpatialized)(ObjectId, bool);
    void (*SetDistance)(ObjectId, float minDistance, float maxDistance);
};

} // namespace anito
