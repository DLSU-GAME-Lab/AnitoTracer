#pragma once

#include "anito/generated/api/Audio.hpp"

namespace anito {
class AudioSource : public generated::AudioSourceHandle {
public:
    using generated::AudioSourceHandle::AudioSourceHandle;
};
}
