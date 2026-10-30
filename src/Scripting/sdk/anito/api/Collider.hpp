#pragma once

#include "Common.hpp"

namespace anito {
class Collider {
public:
    ObjectId id = 0;

    bool valid() const { return detail::H().physics.HasCollider(id); }
    inline GameObject gameObject() const;

    bool operator==(const Collider& o) const { return id == o.id; }
    bool operator!=(const Collider& o) const { return id != o.id; }
};
} // namespace anito
