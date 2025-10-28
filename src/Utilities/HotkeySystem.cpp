#include "HotkeySystem.hpp"
#include "../From-GDGRAP2/Debug.h"

HotkeySystem* HotkeySystem::sharedInstance = nullptr;

HotkeySystem::HotkeySystem()
{
	setupDefaultBindings();
}

HotkeySystem* HotkeySystem::getInstance()
{
	return sharedInstance;
}

void HotkeySystem::initialize()
{
	sharedInstance = new HotkeySystem();
}

void HotkeySystem::destroy()
{
	sharedInstance->m_keyBindings.clear();
}

void HotkeySystem::bindHotkey(EventKey event, Action action)
{
	this->m_keyBindings[event] = action;
}

void HotkeySystem::processInput(int key, int mod)
{
	Debug::Log("Key Pressed: " + std::to_string(key) + " With Mod: " + std::to_string(mod));

	auto it = this->m_keyBindings.find({ key, mod });

	if (it != this->m_keyBindings.end())
	{
		Action action = it->second;
		
		Debug::Log("Action Detected: " + std::to_string(static_cast<int>(action)));

		for (auto listener : this->m_hotkeyListeners)
		{
			listener->OnActionPressed(action);
		}
	}
}

void HotkeySystem::setupDefaultBindings()
{
	using Action = Hotkey::Action;

	// Camera Movement (FPS)
	bindHotkey({ GLFW_KEY_UP, 0 }, Action::Camera_Up);
	bindHotkey({ GLFW_KEY_E, 0 }, Action::Camera_Up);
	bindHotkey({ GLFW_KEY_DOWN, 0 }, Action::Camera_Down);
	bindHotkey({ GLFW_KEY_Q, 0 }, Action::Camera_Down);
	bindHotkey({ GLFW_KEY_LEFT, 0 }, Action::Camera_StrafeLeft);
	bindHotkey({ GLFW_KEY_A, 0 }, Action::Camera_StrafeLeft);
	bindHotkey({ GLFW_KEY_RIGHT, 0 }, Action::Camera_StrafeRight);
	bindHotkey({ GLFW_KEY_D, 0 }, Action::Camera_StrafeRight);
	bindHotkey({ GLFW_KEY_W, 0 }, Action::Camera_Forward);
	bindHotkey({ GLFW_KEY_S, 0 }, Action::Camera_Backward);

	bindHotkey({ GLFW_KEY_W, 0 }, Action::SceneTool_Move);
	bindHotkey({ GLFW_KEY_E, 0 }, Action::SceneTool_Rotate);
	bindHotkey({ GLFW_KEY_R, 0 }, Action::SceneTool_Scale);
	bindHotkey({ GLFW_KEY_Y, 0 }, Action::SceneTool_Transform);
}


