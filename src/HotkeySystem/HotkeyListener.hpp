#pragma once
#include "ActionMap.hpp"

class HotkeyListener
{
public:
	HotkeyListener() = default;
	~HotkeyListener() = default;

	virtual void OnActionPressed(Hotkey::Action action) {}
	virtual void OnActionReleased(Hotkey::Action action) {}
};