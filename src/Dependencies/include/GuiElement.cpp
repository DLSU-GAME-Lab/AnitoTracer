#include "Dependencies/include/GuiElement.h"

void gbe::editor::GuiElement::Draw()
{
	if (!this->ext_Begin())
	{
		this->ext_End();
		return;
	}
	this->DrawSelf();
	this->ext_End();
}
