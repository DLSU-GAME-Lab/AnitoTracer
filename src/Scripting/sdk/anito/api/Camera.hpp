#pragma once

#include "anito/generated/api/Camera.hpp"

namespace anito {
class Camera : public generated::CameraHandle {
public:
    using generated::CameraHandle::CameraHandle;
};
}
