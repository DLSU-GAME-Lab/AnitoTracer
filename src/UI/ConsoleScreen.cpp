#include "ConsoleScreen.h"

#include <imgui_internal.h>
#include <sstream>
void ConsoleScreen::appendText(String text)
{
	std::stringstream buffer;
	buffer << this->lineCount << " " << text;

	this->textLog->appendf(buffer.str().c_str());
	this->lineCount++;
}

ConsoleScreen::ConsoleScreen(): AUIScreen(UINames::CONSOLE_SCREEN)
{
	this->textLog = new ImGuiTextBuffer();
}

ConsoleScreen::~ConsoleScreen()
{
	delete this->textLog;
}

void ConsoleScreen::drawUI()
{
	//setWindowAlignment(ScreenAlign::BOT_CENTER);

	ImGui::Begin("Console", 0, UISettings::GlobalWindowFlags);
	ImGui::SetWindowSize(ImVec2(1200, 300));
	if (ImGui::Button("Clear")) { this->textLog->clear(); this->lineCount = 0; }
	ImGui::TextUnformatted(this->textLog->begin(), this->textLog->end());
	ImGui::End();
}
