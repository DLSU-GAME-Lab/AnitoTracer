#include "pch.h"
#include "GLFWInputAdapter.h"

#include <iostream>

GLFWInputAdapter::GLFWInputAdapter(GLFWwindow* window)
    : m_window(window)
{
    initializeCallbacks();
}

void GLFWInputAdapter::initializeCallbacks()
{
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
}

// -------------------------
//  Keyboard input handling
// -------------------------
void GLFWInputAdapter::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    InputEvent e{};
    e.device = InputDeviceType::Keyboard;
    e.key = mapGLFWKeyToKeyCode(key);
    e.modifiers = mapGLFWModifiers(mods);
    e.action = mapGLFWAction(action);

    HotkeySystem::getInstance()->processInputEvent(e);
}

// -------------------------
//  Mouse input handling
// -------------------------
void GLFWInputAdapter::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    InputEvent e{};
    e.device = InputDeviceType::Mouse;
    e.key = static_cast<KeyCode>(
        (button == GLFW_MOUSE_BUTTON_LEFT) ? KeyCode::LeftMouse :
        (button == GLFW_MOUSE_BUTTON_RIGHT) ? KeyCode::RightMouse :
        (button == GLFW_MOUSE_BUTTON_MIDDLE) ? KeyCode::MiddleMouse : KeyCode::None
        );
    e.modifiers = mapGLFWModifiers(mods);
    e.action = mapGLFWAction(action);

    HotkeySystem::getInstance()->processInputEvent(e);
}

// -------------------------
//  Mapping functions
// -------------------------
KeyCode GLFWInputAdapter::mapGLFWKeyToKeyCode(int key)
{
    switch (key)
    {
        // Letters
    case GLFW_KEY_A: return KeyCode::A;
    case GLFW_KEY_B: return KeyCode::B;
    case GLFW_KEY_C: return KeyCode::C;
    case GLFW_KEY_D: return KeyCode::D;
    case GLFW_KEY_E: return KeyCode::E;
    case GLFW_KEY_F: return KeyCode::F;
    case GLFW_KEY_G: return KeyCode::G;
    case GLFW_KEY_H: return KeyCode::H;
    case GLFW_KEY_I: return KeyCode::I;
    case GLFW_KEY_J: return KeyCode::J;
    case GLFW_KEY_K: return KeyCode::K;
    case GLFW_KEY_L: return KeyCode::L;
    case GLFW_KEY_M: return KeyCode::M;
    case GLFW_KEY_N: return KeyCode::N;
    case GLFW_KEY_O: return KeyCode::O;
    case GLFW_KEY_P: return KeyCode::P;
    case GLFW_KEY_Q: return KeyCode::Q;
    case GLFW_KEY_R: return KeyCode::R;
    case GLFW_KEY_S: return KeyCode::S;
    case GLFW_KEY_T: return KeyCode::T;
    case GLFW_KEY_U: return KeyCode::U;
    case GLFW_KEY_V: return KeyCode::V;
    case GLFW_KEY_W: return KeyCode::W;
    case GLFW_KEY_X: return KeyCode::X;
    case GLFW_KEY_Y: return KeyCode::Y;
    case GLFW_KEY_Z: return KeyCode::Z;

        // Numbers
    case GLFW_KEY_0: return KeyCode::Num0;
    case GLFW_KEY_1: return KeyCode::Num1;
    case GLFW_KEY_2: return KeyCode::Num2;
    case GLFW_KEY_3: return KeyCode::Num3;
    case GLFW_KEY_4: return KeyCode::Num4;
    case GLFW_KEY_5: return KeyCode::Num5;
    case GLFW_KEY_6: return KeyCode::Num6;
    case GLFW_KEY_7: return KeyCode::Num7;
    case GLFW_KEY_8: return KeyCode::Num8;
    case GLFW_KEY_9: return KeyCode::Num9;

        // Arrows
    case GLFW_KEY_UP: return KeyCode::Up;
    case GLFW_KEY_DOWN: return KeyCode::Down;
    case GLFW_KEY_LEFT: return KeyCode::Left;
    case GLFW_KEY_RIGHT: return KeyCode::Right;

        // Function Keys
    case GLFW_KEY_F1: return KeyCode::F1;
    case GLFW_KEY_F2: return KeyCode::F2;
    case GLFW_KEY_F3: return KeyCode::F3;
    case GLFW_KEY_F4: return KeyCode::F4;
    case GLFW_KEY_F5: return KeyCode::F5;
    case GLFW_KEY_F6: return KeyCode::F6;
    case GLFW_KEY_F7: return KeyCode::F7;
    case GLFW_KEY_F8: return KeyCode::F8;
    case GLFW_KEY_F9: return KeyCode::F9;
    case GLFW_KEY_F10: return KeyCode::F10;
    case GLFW_KEY_F11: return KeyCode::F11;
    case GLFW_KEY_F12: return KeyCode::F12;

        // Control Keys
    case GLFW_KEY_ESCAPE: return KeyCode::Escape;
    case GLFW_KEY_TAB: return KeyCode::Tab;
    case GLFW_KEY_CAPS_LOCK: return KeyCode::CapsLock;
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT: return KeyCode::Shift;
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL: return KeyCode::Control;
    case GLFW_KEY_LEFT_ALT:
    case GLFW_KEY_RIGHT_ALT: return KeyCode::Alt;
    case GLFW_KEY_SPACE: return KeyCode::Space;
    case GLFW_KEY_ENTER: return KeyCode::Enter;
    case GLFW_KEY_BACKSPACE: return KeyCode::Backspace;
    case GLFW_KEY_INSERT: return KeyCode::Insert;
    case GLFW_KEY_DELETE: return KeyCode::Delete;
    case GLFW_KEY_HOME: return KeyCode::Home;
    case GLFW_KEY_END: return KeyCode::End;
    case GLFW_KEY_PAGE_UP: return KeyCode::PageUp;
    case GLFW_KEY_PAGE_DOWN: return KeyCode::PageDown;

        // Punctuation
    case GLFW_KEY_MINUS: return KeyCode::Minus;
    case GLFW_KEY_EQUAL: return KeyCode::Equal;
    case GLFW_KEY_LEFT_BRACKET: return KeyCode::LeftBracket;
    case GLFW_KEY_RIGHT_BRACKET: return KeyCode::RightBracket;
    case GLFW_KEY_BACKSLASH: return KeyCode::Backslash;
    case GLFW_KEY_SEMICOLON: return KeyCode::Semicolon;
    case GLFW_KEY_APOSTROPHE: return KeyCode::Apostrophe;
    case GLFW_KEY_COMMA: return KeyCode::Comma;
    case GLFW_KEY_PERIOD: return KeyCode::Period;
    case GLFW_KEY_SLASH: return KeyCode::Slash;
    case GLFW_KEY_GRAVE_ACCENT: return KeyCode::Grave;

        // Numpad
    case GLFW_KEY_KP_0: return KeyCode::Numpad0;
    case GLFW_KEY_KP_1: return KeyCode::Numpad1;
    case GLFW_KEY_KP_2: return KeyCode::Numpad2;
    case GLFW_KEY_KP_3: return KeyCode::Numpad3;
    case GLFW_KEY_KP_4: return KeyCode::Numpad4;
    case GLFW_KEY_KP_5: return KeyCode::Numpad5;
    case GLFW_KEY_KP_6: return KeyCode::Numpad6;
    case GLFW_KEY_KP_7: return KeyCode::Numpad7;
    case GLFW_KEY_KP_8: return KeyCode::Numpad8;
    case GLFW_KEY_KP_9: return KeyCode::Numpad9;
    case GLFW_KEY_KP_DECIMAL: return KeyCode::NumpadDecimal;
    case GLFW_KEY_KP_DIVIDE: return KeyCode::NumpadDivide;
    case GLFW_KEY_KP_MULTIPLY: return KeyCode::NumpadMultiply;
    case GLFW_KEY_KP_SUBTRACT: return KeyCode::NumpadSubtract;
    case GLFW_KEY_KP_ADD: return KeyCode::NumpadAdd;
    case GLFW_KEY_KP_ENTER: return KeyCode::NumpadEnter;

        // Misc
    case GLFW_KEY_PRINT_SCREEN: return KeyCode::PrintScreen;
    case GLFW_KEY_SCROLL_LOCK: return KeyCode::ScrollLock;
    case GLFW_KEY_PAUSE: return KeyCode::Pause;
    case GLFW_KEY_MENU: return KeyCode::Menu;

    default: return KeyCode::None;
    }
}

Modifiers GLFWInputAdapter::mapGLFWModifiers(int mods)
{
    Modifiers result = Modifiers::None;
    if (mods & GLFW_MOD_SHIFT) result |= Modifiers::Shift;
    if (mods & GLFW_MOD_CONTROL) result |= Modifiers::Ctrl;
    if (mods & GLFW_MOD_ALT) result |= Modifiers::Alt;
    if (mods & GLFW_MOD_SUPER) result |= Modifiers::Super;
    return result;
}

InputAction GLFWInputAdapter::mapGLFWAction(int action)
{
    switch (action)
    {
    case GLFW_PRESS: return InputAction::Press;
    case GLFW_RELEASE: return InputAction::Release;
    case GLFW_REPEAT: return InputAction::Repeat;
    default: return InputAction::None;
    }
}
