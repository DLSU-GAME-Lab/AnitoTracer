#pragma once
namespace gbe {

    // Helper Bitwise Operators for KeyModifiers
    inline KeyModifier operator|(KeyModifier lhs, KeyModifier rhs) {
        return static_cast<KeyModifier>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    }

    inline KeyModifier operator&(KeyModifier lhs, KeyModifier rhs) {
        return static_cast<KeyModifier>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
    }
}