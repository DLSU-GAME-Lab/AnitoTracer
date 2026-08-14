#pragma once


#include "Enum_Key.hpp"
#include "Enum_KeyModifier.hpp"
#include "KeyModifierOperators.hpp"

#include "EventSystem.hpp"
namespace gbe {

    enum class InputTrigger {
        Down,   // Fired on the frame the key was pressed
        Up,     // Fired on the frame the key was released
        While   // Fired every frame while the key is held
    };

    // Event Payload sent when input triggers
    struct InputEventArgs : public EventArgs {
        std::string actionName;
        Key key;
        InputTrigger trigger;
        KeyModifier modifiers;

        InputEventArgs(std::string action, Key k, InputTrigger t, KeyModifier m)
            : actionName(std::move(action)), key(k), trigger(t), modifiers(m) {
        }
    };
}