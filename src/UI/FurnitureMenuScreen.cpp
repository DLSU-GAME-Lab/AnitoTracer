#include "FurnitureMenuScreen.h"

FurnitureMenuScreen::FurnitureMenuScreen() : AUIScreen(UINames::FURNITURE_MENU_SCREEN)
{
}

void FurnitureMenuScreen::drawUI()
{

	if (ImGui::Begin("Assets"))
	{

		static float padding = 16.0f;
		static float thumbnailSize = 128.0f;
		float cellSize = thumbnailSize + padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, 0, false);

		ImDrawList* list = ImGui::GetWindowDrawList();
		for (int i = 0; i < 100; i++)
		{
			ImGui::PushID(i);

			ImGui::ImageButton((ImTextureID)0, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });

			ImGui::NextColumn();
			ImGui::PopID();
		}

		ImGui::End();
	}
}
