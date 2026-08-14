#pragma once

namespace gbe {

    enum class KeyModifier : uint8_t {
        None = 0,
        Shift = 1 << 0,
        Ctrl = 1 << 1,
        Alt = 1 << 2
    };
}