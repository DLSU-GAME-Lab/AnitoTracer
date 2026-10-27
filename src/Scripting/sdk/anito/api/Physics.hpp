#pragma once

#include "Common.hpp"
#include "RaycastHit.hpp"

namespace anito {
struct Physics {
    static Vec3 gravity() { Vec3 g{}; detail::H().physics.GetGravity(&g); return g; }
    static void setGravity(Vec3 g) { detail::H().physics.SetGravity(g); }
    inline static RaycastHit raycast(Vec3 origin, Vec3 direction, float maxDistance, GameObject ignore);
};
} // namespace anito
