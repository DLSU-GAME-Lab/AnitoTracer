#pragma once

#include "Common.hpp"

namespace anito {
class RaycastHit {
public:
    RaycastResult result{};

    bool hit() const { return result.hit != 0; }
    Vec3 point() const { return result.point; }
    inline GameObject gameObject() const;
};
} // namespace anito
