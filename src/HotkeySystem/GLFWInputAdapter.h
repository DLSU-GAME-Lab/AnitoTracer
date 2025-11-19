#pragma once
//#include <GLFW/glfw3.h>
#include "InputEvent.h"
#include "HotkeySystem.hpp"

class GLFWInputAdapter
{
public:
    GLFWInputAdapter(GLFWwindow* window);
    ~GLFWInputAdapter() = default;

    void initializeCallbacks();

private:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    static KeyCode mapGLFWKeyToKeyCode(int key);
    static Modifiers mapGLFWModifiers(int mods);
    static InputAction mapGLFWAction(int action);

    GLFWwindow* m_window;
};

