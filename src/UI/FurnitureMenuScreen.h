#pragma once
#include <imgui_internal.h>
#include <imgui.h>

#include "AUIScreen.h"
class FurnitureMenuScreen :
	public AUIScreen
{
public:
	FurnitureMenuScreen();
	void drawUI() override;
};

