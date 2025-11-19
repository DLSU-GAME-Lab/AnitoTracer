#include "pch.h"
#include "HotkeySystem.hpp"
#include "InputEvent.h"
#include "Debug.h"

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

void HotkeySystem::processInputEvent(const InputEvent& event)
{
	EventKey ek{ event.key, event.modifiers };

    HotkeySystem::BindingTable bindings;

    switch (event.device)
    {
        case InputDeviceType::Keyboard: bindings = m_keyBindings; break;
        case InputDeviceType::Mouse:    bindings = m_mouseButtonBindings; break;
        case InputDeviceType::Gamepad:  bindings = m_gamepadButtonBindings; break;
        default: return;
    }

	auto it = bindings.find(ek);

	if (it != bindings.end()) 
	{
		for (auto& action : it->second) 
		{
			for (auto* listener : m_hotkeyListeners)
				if(event.action == InputAction::Press)
					listener->OnActionPressed(action);
				else if (event.action == InputAction::Release)
					listener->OnActionReleased(action);
		}
	}
}

void HotkeySystem::setupDefaultBindings()
{
    using Action = Hotkey::Action;

    bindHotkey({ KeyCode::Up, Modifiers::None }, Action::Camera_Forward);
    bindHotkey({ KeyCode::E, Modifiers::None }, Action::Camera_Up);
    bindHotkey({ KeyCode::Down, Modifiers::None }, Action::Camera_Backward);
    bindHotkey({ KeyCode::Q, Modifiers::None }, Action::Camera_Down);
    bindHotkey({ KeyCode::Left, Modifiers::None }, Action::Camera_StrafeLeft);
    bindHotkey({ KeyCode::A, Modifiers::None }, Action::Camera_StrafeLeft);
    bindHotkey({ KeyCode::Right, Modifiers::None }, Action::Camera_StrafeRight);
    bindHotkey({ KeyCode::D, Modifiers::None }, Action::Camera_StrafeRight);
    bindHotkey({ KeyCode::W, Modifiers::None }, Action::Camera_Forward);
    bindHotkey({ KeyCode::S, Modifiers::None }, Action::Camera_Backward);

    bindHotkey({ KeyCode::W, Modifiers::None }, Action::SceneTool_Move);
    bindHotkey({ KeyCode::E, Modifiers::None }, Action::SceneTool_Rotate);
    bindHotkey({ KeyCode::R, Modifiers::None }, Action::SceneTool_Scale);
    bindHotkey({ KeyCode::T, Modifiers::None }, Action::SceneTool_Transform);
    bindHotkey({ KeyCode::G, Modifiers::None }, Action::SceneTool_Cycle);

    bindHotkey({ KeyCode::A, Modifiers::Shift | Modifiers::Alt }, Action::Toggle_GameObjectEnabled);
    bindHotkey({ KeyCode::Delete, Modifiers::None }, Action::DeleteGameObject);
    bindHotkey({ KeyCode::D, Modifiers::Ctrl }, Action::DuplicateGameObject);
    bindHotkey({ KeyCode::C, Modifiers::Ctrl }, Action::CopyGameObject);
    bindHotkey({ KeyCode::V, Modifiers::Ctrl }, Action::PasteGameObject);
    bindHotkey({ KeyCode::X, Modifiers::Ctrl }, Action::CutGameObject);

    bindHotkey({ KeyCode::Equal, Modifiers::Ctrl }, Action::Hierarchy_SetAsFirstSibling);
    bindHotkey({ KeyCode::Minus, Modifiers::Ctrl }, Action::Hierarchy_SetAsLastSibling);
    bindHotkey({ KeyCode::H, Modifiers::None }, Action::Hierarchy_ToggleVisibilityWithDescendants);
    bindHotkey({ KeyCode::L, Modifiers::None }, Action::Hierarchy_TogglePickabilityWithDescendants);

    bindHotkey({ KeyCode::F7, Modifiers::None }, Action::Camera_Reset);

    bindHotkey({ KeyCode::F, Modifiers::Ctrl | Modifiers::Alt }, Action::GameObject_MoveToView);

    // Mouse binds (also need agnostic enum)
    bindMouseHotkey({ KeyCode::RightMouse, Modifiers::None }, Action::Camera_FPSMode);
    bindMouseHotkey({ KeyCode::MiddleMouse, Modifiers::None }, Action::Camera_NormalPanMode);
    bindMouseHotkey({ KeyCode::LeftMouse, Modifiers::Ctrl | Modifiers::Alt }, Action::Camera_NormalPanMode);
    bindMouseHotkey({ KeyCode::MiddleMouse, Modifiers::Shift }, Action::Camera_FastPanMode);
    bindMouseHotkey({ KeyCode::MiddleMouse, Modifiers::Alt }, Action::Camera_SlowPanMode);
    bindMouseHotkey({ KeyCode::RightMouse, Modifiers::Alt }, Action::Camera_ZoomMode);
    bindMouseHotkey({ KeyCode::LeftMouse, Modifiers::Alt }, Action::Camera_OrbitMode);

	bindHotkey({ KeyCode::Z, Modifiers::Ctrl }, Action::Undo);
	bindHotkey({ KeyCode::Y, Modifiers::Ctrl }, Action::Redo);
}



