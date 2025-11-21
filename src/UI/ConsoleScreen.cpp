#include "ConsoleScreen.h"
#include "UIManager.h"
#include "IconsMaterialDesign.h"
#include <imgui_internal.h>
#include <sstream>
void ConsoleScreen::appendText(String text)
{
	std::stringstream buffer;
	buffer << this->lineCount << " " << text;

	this->textLog->appendf(buffer.str().c_str());
	this->lineCount++;
}

void ConsoleScreen::setText(String log, int lineCount)
{
	this->textLog->clear();
	this->textLog->appendf(log.c_str());
	this->lineCount = lineCount;
}

ConsoleScreen::ConsoleScreen() : AUIScreen(UINames::CONSOLE_SCREEN)
{
	this->textLog = new ImGuiTextBuffer();
}

ConsoleScreen::~ConsoleScreen()
{
	String currentLog = this->textLog->c_str();
	UIManager::getInstance()->config()->consoleTextLog = currentLog; // save logs before destruction
	UIManager::getInstance()->config()->consoleLogCount = this->lineCount;
	delete this->textLog;
}

void ConsoleScreen::drawUI()
{
	//setWindowAlignment(ScreenAlign::BOT_CENTER);

	ImGui::Begin(ICON_MD_TEXT_SNIPPET " Console", 0, UISettings::GlobalWindowFlags);
	ImGui::SetWindowSize(ImVec2(1200, 300));
	if (ImGui::Button("Clear")) { this->textLog->clear(); this->lineCount = 0; }

	ImGui::Separator();

	if (ImGui::BeginChild("scrolling", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
	{
		//ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
		//ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
		ImGui::TextUnformatted(this->textLog->begin(), this->textLog->end());
		//ImGui::PopStyleVar();

		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
			ImGui::SetScrollHereY(1.0f);

	}
	ImGui::EndChild();
	ImGui::End();
}
