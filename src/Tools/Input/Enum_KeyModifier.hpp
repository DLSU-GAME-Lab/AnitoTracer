#pragma once

namespace gbe {

    enum class KeyModifier : uint8_t {
        None = 0,
        Shift = 1 << 0,
        Ctrl = 1 << 1,
        Alt = 1 << 2
    };

    // Helper Bitwise Operators for KeyModifiers
    inline KeyModifier operator|(KeyModifier lhs, KeyModifier rhs) {
        return static_cast<KeyModifier>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    }

    inline KeyModifier operator&(KeyModifier lhs, KeyModifier rhs) {
        return static_cast<KeyModifier>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
    }
}