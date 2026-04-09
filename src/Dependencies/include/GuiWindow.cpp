#include "Dependencies/include/GuiWindow.h"

bool gbe::editor::GuiWindow::ext_Begin() {

	this->push_styles();
	bool began = ImGui::Begin(this->GetWindowId().c_str(), &is_open);
	
	const float label_width_base = ImGui::GetFontSize() * 12;               // Some amount of width for label, based on font size.
	const float label_width_max = ImGui::GetContentRegionAvail().x * 0.40f; // ...but always leave some room for framed widgets.
	const float label_width = label_width_base < label_width_max ? label_width_base : label_width_max;
	ImGui::PushItemWidth(-label_width);

	if (!began) {
		return false;
	}

	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu(this->GetWindowId().c_str()))
		{
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}


	// Get the current mouse position in screen coordinates
	ImVec2 mouse_pos = ImGui::GetIO().MousePos;
	ImVec2 content_min = ImGui::GetCursorScreenPos();
	ImVec2 content_size = ImGui::GetContentRegionAvail();
	ImVec2 content_max = ImVec2(content_min.x + content_size.x, content_min.y + content_size.y);

	// Perform the check
	this->pointer_here = ImGui::IsWindowHovered();

	return true;
}

void gbe::editor::GuiWindow::ext_End() {
	ImGui::PopItemWidth();
	ImGui::End();
	this->pop_styles();
}