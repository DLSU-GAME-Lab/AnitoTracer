#include "GUICommands.hpp"
#include "imgui.h"


ModifyLayoutCommand::ModifyLayoutCommand(const std::string& beforeSnapshot, const std::string& afterSnapshot)
	: m_before(beforeSnapshot), m_after(afterSnapshot)
{
}

void ModifyLayoutCommand::execute()
{
	ImGui::LoadIniSettingsFromMemory(m_after.c_str(), m_after.size());
}

void ModifyLayoutCommand::undo()
{
	ImGui::LoadIniSettingsFromMemory(m_before.c_str(), m_before.size());
}
