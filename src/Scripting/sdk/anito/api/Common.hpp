#pragma once

// Shared declarations and host access for script-facing component handles.

#include "../Host.hpp"

#include <string>

namespace anito {

class GameObject;
class Transform;
class Rigidbody;
class Collider;
class AudioSource;
class Script;

namespace detail {
inline const HostAPI& H() { return Host(); }
} // namespace detail

} // namespace anito
