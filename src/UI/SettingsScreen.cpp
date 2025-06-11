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
	ImGui::SetNextWindowBgAlpha(0.7f); // Transparent background

	if (ImGui::Begin("Settings", &enabled, UISettings::GlobalWindowFlags))
	{
		std::vector<const char*> scenes;
		scenes.reserve(SceneList::AllScenes.size());
		for (const auto& scene : SceneList::AllScenes)
		{
			scenes.push_back(std::get<0>(scene).c_str());
		}

		UserSettings* settings = UIManager::getInstance()->settings();

		ImGui::Text("AnitoTracer Shortcuts");
		ImGui::Separator();
		ImGui::BulletText("F1: Toggle Settings");
		ImGui::BulletText("F2: Toggle Statistics");
		ImGui::BulletText("F3: Toggle All UI");
		ImGui::Separator();
		ImGui::BulletText("Left Click Scene and Hold RMB & WASD/Q/E: Move Camera");
		ImGui::BulletText("Left Click Scene and Hold RMB and drag: look around");
		ImGui::BulletText("F: Focus on selected object");
		ImGui::Separator();
		ImGui::BulletText("F4: Enable/disable ray tracing");
		ImGui::BulletText("F6: Enable/disable ray visualization");
		ImGui::NewLine();
		ImGui::TextWrapped("Tip: Click in the scene to use shortcuts in the scene");
		ImGui::NewLine();

		ImGui::Text("Scene");
		ImGui::Separator();
		ImGui::PushItemWidth(-1);
		ImGui::Combo("##SceneList", &settings->SceneIndex, scenes.data(), static_cast<int>(scenes.size()));
		ImGui::PopItemWidth();
		ImGui::NewLine();

		ImGui::Text("Ray Tracing");
		ImGui::Separator();
		/*ImGui::Checkbox("Enable ray tracing", &settings->IsRayTraced);*/
		//ImGui::Text("Press T to enable/disable ray tracing");
		ImGui::TextWrapped("Tip: To visualize rays, you must be in ray visualization mode.");
		ImGui::Checkbox("Accumulate rays between frames", &settings->AccumulateRays);
		uint32_t min = 1, max = 512;
		ImGui::SliderScalarN("Samples", ImGuiDataType_U32, &settings->NumberOfSamples, 1, &min, &max);
		min = 1; max = 32;
		ImGui::SliderScalar("Bounces", ImGuiDataType_U32, &settings->NumberOfBounces, &min, &max);
		ImGui::NewLine();

		ImGui::Text("Ray Visualization");
		min = 1; max = 128;
		ImGui::SliderScalarN("Max Rays", ImGuiDataType_U32, &settings->MaxRays, 1, &min, &max);

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
