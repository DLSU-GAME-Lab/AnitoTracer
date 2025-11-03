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

void HotkeySystem::addListener(HotkeyListener* listener)
{
	this->m_hotkeyListeners.insert(listener);
}

void HotkeySystem::removeListener(HotkeyListener* listener)
{
	this->m_hotkeyListeners.erase(listener);
}

void HotkeySystem::bindHotkey(EventKey event, Action action)
{
	this->m_keyBindings[event].push_back(action);
}

void HotkeySystem::bindMouseHotkey(EventKey event, Action action)
{
	this->m_mouseButtonBindings[event].push_back(action);
}

void HotkeySystem::processInputKeys(int key, int mod, int action)
{
	auto it = this->m_keyBindings.find({ key, mod });

	if (it != this->m_keyBindings.end())
	{
		auto keyActions = it->second;

		for (auto keyAction : keyActions)
		{
			for (auto listener : this->m_hotkeyListeners)
			{
				if (action != GLFW_RELEASE)
					listener->OnActionPressed(keyAction);
				else if (action == GLFW_RELEASE)
					listener->OnActionReleased(keyAction);
			}
		}
	}
}

void HotkeySystem::processInputMouseButtons(int key, int mod, int action)
{
	auto it = this->m_mouseButtonBindings.find({ key, mod });

	if (it != this->m_mouseButtonBindings.end())
	{
		auto buttonActions = it->second;

		for (auto buttonAction : buttonActions)
		{
			for (auto listener : this->m_hotkeyListeners)
			{
				if (action != GLFW_RELEASE)
					listener->OnActionPressed(buttonAction);
				else if (action == GLFW_RELEASE)
					listener->OnActionReleased(buttonAction);
			}
		}
	}
}

void HotkeySystem::setupDefaultBindings()
{
	using Action = Hotkey::Action;

	// Camera Movement (FPS)
	bindHotkey({ GLFW_KEY_UP, 0 }, Action::Camera_Forward);
	bindHotkey({ GLFW_KEY_E, 0 }, Action::Camera_Up);
	bindHotkey({ GLFW_KEY_DOWN, 0 }, Action::Camera_Backward);
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
	bindHotkey({ GLFW_KEY_T, 0 }, Action::SceneTool_Transform);
	bindHotkey({ GLFW_KEY_G, 0 }, Action::SceneTool_Cycle);

	bindHotkey({ GLFW_KEY_A, GLFW_MOD_SHIFT + GLFW_MOD_ALT }, Action::Toggle_GameObjectEnabled);
	bindHotkey({ GLFW_KEY_DELETE, 0 }, Action::DeleteGameObject);
	bindHotkey({ GLFW_KEY_D, GLFW_MOD_CONTROL }, Action::DuplicateGameObject);
	bindHotkey({ GLFW_KEY_C, GLFW_MOD_CONTROL }, Action::CopyGameObject);
	bindHotkey({ GLFW_KEY_V, GLFW_MOD_CONTROL }, Action::PasteGameObject);
	bindHotkey({ GLFW_KEY_X, GLFW_MOD_CONTROL }, Action::CutGameObject);

	bindHotkey({ GLFW_KEY_EQUAL, GLFW_MOD_CONTROL }, Action::Hierarchy_SetAsFirstSibling);
	bindHotkey({ GLFW_KEY_MINUS, GLFW_MOD_CONTROL }, Action::Hierarchy_SetAsLastSibling);
	bindHotkey({ GLFW_KEY_H, 0 }, Action::Hierarchy_ToggleVisibilityWithDescendants);
	bindHotkey({ GLFW_KEY_L, 0 }, Action::Hierarchy_TogglePickabilityWithDescendants);

	bindHotkey({ GLFW_KEY_F7, 0 }, Action::Camera_Reset);

	bindHotkey({ GLFW_KEY_F, GLFW_MOD_CONTROL + GLFW_MOD_ALT }, Action::GameObject_MoveToView);

	// Mouse Binds
	bindMouseHotkey({ GLFW_MOUSE_BUTTON_RIGHT, 0 }, Action::Camera_FPSMode);
	bindMouseHotkey({ GLFW_MOUSE_BUTTON_MIDDLE, 0 }, Action::Camera_NormalPanMode);
	bindMouseHotkey({ GLFW_MOUSE_BUTTON_LEFT, GLFW_MOD_CONTROL + GLFW_MOD_ALT }, Action::Camera_NormalPanMode);
	bindMouseHotkey({ GLFW_MOUSE_BUTTON_MIDDLE, GLFW_MOD_SHIFT }, Action::Camera_FastPanMode);
	bindMouseHotkey({ GLFW_MOUSE_BUTTON_MIDDLE, GLFW_MOD_ALT }, Action::Camera_SlowPanMode);
	bindMouseHotkey({ GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOD_ALT }, Action::Camera_ZoomMode);
	bindMouseHotkey({ GLFW_MOUSE_BUTTON_LEFT, GLFW_MOD_ALT }, Action::Camera_OrbitMode);


	bindMouseHotkey({ GLFW_KEY_F, 0 }, Action::Camera_FocusOnGameObject_Zoomed);

}


