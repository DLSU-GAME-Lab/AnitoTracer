#include "SettingsScreen.h"

#include "SceneList.hpp"
#include "UIManager.h"

SettingsScreen::SettingsScreen(): AUIScreen(UINames::SETTINGS_SCREEN)
{
	enabled = true;
}

SettingsScreen::~SettingsScreen() = default;

void SettingsScreen::drawUI()
{
	//setWindowAlignment(ScreenAlign::TOP_LEFT);

	if (ImGui::Begin("Settings", &enabled, UISettings::GlobalWindowFlags))
	{
		std::vector<const char*> scenes;
		scenes.reserve(SceneList::AllScenes.size());
		for (const auto& scene : SceneList::AllScenes)
		{
			scenes.push_back(std::get<0>(scene).c_str());
		}

		UserSettings* settings = UIManager::getInstance()->settings();

		ImGui::Text("Help");
		ImGui::Separator();
		ImGui::BulletText("F1: toggle Settings.");
		ImGui::BulletText("F2: toggle Statistics.");
		ImGui::BulletText("WASD/Q/E: move camera.");
		ImGui::NewLine();

		ImGui::Text("Scene");
		ImGui::Separator();
		ImGui::PushItemWidth(-1);
		ImGui::Combo("##SceneList", &settings->SceneIndex, scenes.data(), static_cast<int>(scenes.size()));
		ImGui::PopItemWidth();
		ImGui::NewLine();

		ImGui::Text("Ray Tracing");
		ImGui::Separator();
		ImGui::Checkbox("Enable ray tracing", &settings->IsRayTraced);
		ImGui::Text("Press T to enable/disable ray tracing");
		ImGui::Checkbox("Accumulate rays between frames", &settings->AccumulateRays);
		uint32_t min = 1, max = 512;
		ImGui::SliderScalarN("Samples", ImGuiDataType_U32, &settings->NumberOfSamples, 1, &min, &max);
		min = 1; max = 32;
		ImGui::SliderScalar("Bounces", ImGuiDataType_U32, &settings->NumberOfBounces, &min, &max);
		ImGui::NewLine();

		ImGui::Text("Camera");
		ImGui::Separator();
		ImGui::SliderFloat("FoV", &settings->FieldOfView, UserSettings::FieldOfViewMinValue, UserSettings::FieldOfViewMaxValue, "%.0f");
		ImGui::SliderFloat("Aperture", &settings->Aperture, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("Focus", &settings->FocusDistance, 0.1f, 20.0f, "%.1f");
		ImGui::NewLine();

		ImGui::Text("Profiler");
		ImGui::Separator();
		ImGui::Checkbox("Show heatmap", &settings->ShowHeatmap);
		ImGui::SliderFloat("Scaling", &settings->HeatmapScale, 0.10f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		ImGui::NewLine();
	}
	ImGui::End();
}
