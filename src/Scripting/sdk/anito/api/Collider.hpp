#pragma once

#include "anito/generated/api/Physics.hpp"

namespace anito {
class Collider : public generated::ColliderHandle {
public:
    using generated::ColliderHandle::ColliderHandle;
};
}
