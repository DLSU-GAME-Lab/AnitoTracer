#pragma once

#include "anito/generated/api/Physics.hpp"

namespace anito {
class Rigidbody : public generated::RigidbodyHandle {
public:
    using generated::RigidbodyHandle::RigidbodyHandle;
};
}
