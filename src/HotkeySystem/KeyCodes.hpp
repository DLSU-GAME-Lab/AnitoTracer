#pragma once
#include <type_traits>

enum class KeyCode : int
{
    None = 0,

    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Arrow Keys
    Up, Down, Left, Right,

    // Function Keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Control Keys
    Escape, Tab, CapsLock, Shift, Control, Alt, Space, Enter, Backspace,
    Insert, Delete, Home, End, PageUp, PageDown,

    // Punctuation / Symbols
    Minus, Equal, LeftBracket, RightBracket, Backslash,
    Semicolon, Apostrophe, Comma, Period, Slash, Grave,

    // Numpad Keys
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
	NumpadDecimal, NumpadDivide, NumpadMultiply, NumpadSubtract, NumpadAdd, NumpadEnter, //numpadEnter not supported everywhere

    // Special / Misc
    PrintScreen, ScrollLock, Pause, Menu,

	// Mouse Buttons
    LeftMouse, RightMouse, MiddleMouse,
};

enum class Modifiers : uint8_t
{
    None = 0,
    Shift = 1 << 0,
    Ctrl = 1 << 1,
    Alt = 1 << 2,
    Super = 1 << 3, // Windows key / Command key
};

inline Modifiers operator|(Modifiers a, Modifiers b)
{
    return static_cast<Modifiers>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline Modifiers operator&(Modifiers a, Modifiers b)
{
    return static_cast<Modifiers>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline Modifiers& operator|=(Modifiers& a, Modifiers b)
{
    a = a | b;
    return a;
}

inline Modifiers& operator&=(Modifiers& a, Modifiers b)
{
    a = a & b;
    return a;
}

// Helper to check if a modifier is set
inline bool hasModifier(Modifiers mods, Modifiers check)
{
    return (mods & check) != Modifiers::None;
}


