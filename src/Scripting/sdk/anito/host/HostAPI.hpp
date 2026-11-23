#pragma once

#include "anito/generated/host/HostIncludes.hpp"

namespace anito {
struct HostAPI {
#include "anito/generated/host/HostFields.inc"
};

namespace detail {
// One pointer per loaded module; set by ScriptBehaviour's constructor before any script code runs.
inline const HostAPI*& HostPtr() {
    static const HostAPI* host = nullptr;
    return host;
}
inline const HostAPI& Host() { return *HostPtr(); }
} // namespace detail

} // namespace anito
