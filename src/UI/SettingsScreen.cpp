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
		ImGui::Bullet();
		ImGui::TextWrapped("F1: Toggle Settings");
		ImGui::Bullet();
		ImGui::TextWrapped("F2: Toggle Statistics");
		ImGui::Bullet();
		ImGui::TextWrapped("F3: Toggle All UI");
		ImGui::Separator();
		ImGui::Bullet();
		ImGui::TextWrapped("Left Click Scene and Hold RMB + WASD/Q/E: Move Camera");
		ImGui::Bullet();
		ImGui::TextWrapped("Left Click Scene and Hold RMB + drag: Look Around");
		ImGui::Bullet();
		ImGui::TextWrapped("Select an Object + W: Move Object Gizmo");
		ImGui::Bullet();
		ImGui::TextWrapped("Select an Object + E: Rotate Object Gizmo");
		ImGui::Bullet();
		ImGui::TextWrapped("Select an Object + R: Scale Object Gizmo");
		ImGui::Bullet();
		ImGui::TextWrapped("F: Focus on selected object");
		ImGui::Bullet();
		ImGui::TextWrapped("F5: Refresh Scene to see your changes");
		ImGui::Separator();
		ImGui::Bullet();
		ImGui::TextWrapped("F4: Enable/Disable ray tracing");
		ImGui::Bullet();
		ImGui::TextWrapped("F6: Enable/Disable ray visualization");
		ImGui::NewLine();
		ImGui::TextWrapped("Tip: Click in the scene to use shortcuts in the scene");
		ImGui::NewLine();

		//ImGui::Text("Scene");
		//ImGui::Separator();
		//ImGui::PushItemWidth(-1);
		//ImGui::Combo("##SceneList", &settings->SceneIndex, scenes.data(), static_cast<int>(scenes.size()));
		//ImGui::PopItemWidth();
		//ImGui::NewLine();

		ImGui::Text("Ray Tracing");
		ImGui::Separator();
		/*ImGui::Checkbox("Enable ray tracing", &settings->IsRayTraced);*/
		//ImGui::Text("Press T to enable/disable ray tracing");
		ImGui::TextWrapped("Tip: To visualize rays, you must be in ray visualization mode.");
		ImGui::Checkbox("Accumulate rays between frames", &settings->AccumulateRays);
		ImGui::Checkbox("Multisample Anti-Aliasing", &settings->MultiSampling);
		uint32_t min = 2, max = 8;
		std::string label = (std::to_string(settings->aaValue) + "x");
		if (ImGui::SliderScalarN("MSAA Value", ImGuiDataType_U32, &settings->aaValue, 1, &min, &max, label.c_str())) { // if value has changed
			settings->aaValue = (settings->aaValue / 2) * 2; // round to even number
		}
		min = 1, max = 24;
		ImGui::SliderScalarN("Samples", ImGuiDataType_U32, &settings->NumberOfSamples, 1, &min, &max);
		min = 2; max = 6;
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
