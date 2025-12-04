#pragma once
#include "KeyCodes.hpp"

enum class InputDeviceType
{
    Keyboard,
    Mouse,
    Gamepad,
    Touch
};

enum class InputAction
{
    None,
    Press,
    Release,
    Repeat
};

struct InputEvent
{
    InputDeviceType device;
    KeyCode key;       
    Modifiers modifiers;
    InputAction action;
};


