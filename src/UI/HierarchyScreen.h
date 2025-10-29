#pragma once
#include <unordered_set>

#include "AUIScreen.h"
#include "From-GDGRAP2/GameObject.h"
#include "Utilities/HotkeyListener.hpp"

class HierarchyScreen : public AUIScreen, public HotkeyListener
{
public:
	HierarchyScreen();
	~HierarchyScreen();

	void OnActionPressed(Hotkey::Action action) override;

private:
	virtual void drawUI() override;
	void updateObjectList(const char* filter);
	void drawObjectNode(GameObject* obj);

	mutable std::unordered_set<std::string> openNodes;

	bool isDragging = false;

	friend class UIManager;
};

