#pragma once


#include "Enum_Key.hpp"
#include "Enum_KeyModifier.hpp"

#include "EventSystem.hpp"

namespace gbe {

    // InputTriggers transformed into a Bitmask
    enum class InputTrigger : uint8_t {
        None = 0,
        Down = 1 << 0, // Frame key is pressed
        While = 1 << 1, // Every frame key is held
        Up = 1 << 2, // Frame key is released
        All = Down | While | Up
    };

    inline InputTrigger operator|(InputTrigger lhs, InputTrigger rhs) {
        return static_cast<InputTrigger>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    }
    inline InputTrigger operator&(InputTrigger lhs, InputTrigger rhs) {
        return static_cast<InputTrigger>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
    }

    struct InputEventArgs : public EventArgs {
        std::string actionName;
        Key key;
        InputTrigger currentPhase; // The specific phase triggering this event
        KeyModifier modifiers;

        InputEventArgs(std::string action, Key k, InputTrigger phase, KeyModifier m)
            : actionName(std::move(action)), key(k), currentPhase(phase), modifiers(m) {}
    };
}