#pragma once

namespace gbe {
    enum class Key {
        // Alphanumerics
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

        // Controls & Cursors
        Space, Enter, Escape, Tab, Backspace,
        Up, Down, Left, Right,

        // Mouse Buttons
        MouseLeft, MouseRight, MouseMiddle, Mouse4, Mouse5,

        COUNT
    };
}