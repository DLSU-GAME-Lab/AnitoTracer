#include "AUIScreen.h"

String AUIScreen::getName()
{
	return this->name;
}

void AUIScreen::setWindowAlignment(const ScreenAlign& alignment, const ImGuiCond& condition)
{
	constexpr float yOffset = 25.f;
	ImVec2 pos = { 0, 0.f }; // Next window position
	ImVec2 piv = { 0.f, 0.f }; // Next window pivot
	const ImVec2 win = ImGui::GetIO().DisplaySize; // Size of application window

	switch (alignment)
	{
	case ScreenAlign::TOP_LEFT:
		pos.y = yOffset; // offset Y a little for the toolbar
		break;
	case ScreenAlign::TOP_RIGHT:
		pos = { win.x, yOffset };
		piv.x = 1.f;
		break;
	case ScreenAlign::TOP_CENTER:
		pos = { win.x / 2.f, yOffset };
		piv.x = 0.5f;
		break;
	case ScreenAlign::CENTER_LEFT:
		pos.y = win.y / 2.f;
		piv.y = 0.5f;
		break;
	case ScreenAlign::CENTER_RIGHT:
		pos = { win.x, win.y / 2.f };
		piv = { 1.f, 0.5f };
		break;
	case ScreenAlign::CENTER:
		pos = { win.x / 2.f, win.y / 2.f };
		piv = { 0.5f, 0.5f };
		break;
	case ScreenAlign::BOT_LEFT:
		pos.y = win.y / 2.f;
		piv = { 0.5f, 0.5f };
		break;
	case ScreenAlign::BOT_RIGHT:
		pos = { win.x, win.y };
		piv = { 1.f, 1.0f };
		break;
	case ScreenAlign::BOT_CENTER:
		pos = {win.x / 2.f, win.y};
		piv = { 1.f, 1.0f };
		break;
	}

	ImGui::SetNextWindowPos(pos, condition, piv);
}

void AUIScreen::setEnabled(const bool flag)
{
	this->enabled = flag;
}

void AUIScreen::toggleEnabled()
{
	this->enabled = !this->enabled;
}

AUIScreen::AUIScreen(const String& name)
{
	this->name = name;
}

AUIScreen::~AUIScreen()
= default;
