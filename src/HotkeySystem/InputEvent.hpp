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
    None = -1,
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


