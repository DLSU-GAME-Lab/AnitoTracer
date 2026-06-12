#include "SettingsScreen.h"
#include "../src/Engine/CameraSystem/CameraManager.h"
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

		ImGui::Text("AnitoTracer Controls");
		ImGui::Separator();
		ImGui::Bullet();
		ImGui::TextWrapped("F1: Toggle Settings");
		ImGui::Bullet();
		ImGui::TextWrapped("F2: Toggle Statistics");
		ImGui::Bullet();
		ImGui::TextWrapped("F3: Toggle All UI");
		ImGui::Bullet();
		ImGui::TextWrapped("F4: Enable/Disable ray tracing");
		ImGui::Bullet();
		ImGui::TextWrapped("F5: Refresh Scene to see your changes");
		ImGui::Bullet();
		ImGui::TextWrapped("F6: Enable/Disable ray visualization");
		ImGui::Separator();

		ImGui::Bullet();
		ImGui::TextWrapped("Left Click Object To Select");
		ImGui::Bullet();
		ImGui::TextWrapped("Left Click Scene and Hold RMB: FPS Mode");
		ImGui::Bullet();
		ImGui::TextWrapped("Select an Object + W: Move Object Gizmo");
		ImGui::Bullet();
		ImGui::TextWrapped("Select an Object + E: Rotate Object Gizmo");
		ImGui::Bullet();
		ImGui::TextWrapped("Select an Object + R: Scale Object Gizmo");
		ImGui::Bullet();
		ImGui::TextWrapped("Select an Object + T: All Operations Gizmo");
		ImGui::Separator();

		ImGui::TextWrapped("Tip: Click in the scene to use shortcuts in the scene");
		ImGui::Bullet();
		ImGui::TextWrapped("Cut: CTRL + X");
		ImGui::Bullet();
		ImGui::TextWrapped("Copy: CTRL + C");
		ImGui::Bullet();
		ImGui::TextWrapped("Paste: CTRL + V");
		ImGui::Bullet();
		ImGui::TextWrapped("Delete: DEL");
		ImGui::Bullet();
		ImGui::TextWrapped("Undo: CTRL + Z");
		ImGui::Bullet();
		ImGui::TextWrapped("Redo: CTRL + Y");
		ImGui::Bullet();
		ImGui::Separator();

		ImGui::Bullet();
		ImGui::TextWrapped("Drag and Drop OBJ files unto the screen to add more files");

		//ImGui::Text("Scene");
		//ImGui::Separator();
		//ImGui::PushItemWidth(-1);
		//ImGui::Combo("##SceneList", &settings->SceneIndex, scenes.data(), static_cast<int>(scenes.size()));
		//ImGui::PopItemWidth();
		//ImGui::NewLine();

		ImGui::Separator();
		const bool isGameMode = (settings->CurrentRendererMode == UserSettings::RendererMode::Game);

		// ── Ray Tracing settings (hidden in Game mode) ─────────────────────────
		if (!isGameMode)
		{
			ImGui::Text("Ray Tracing");
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
			uint32_t spiMin = 1, spiMax = 512;
			ImGui::SliderScalar("Samples Per Dispatch", ImGuiDataType_U32, &settings->SamplesPerInvocation, &spiMin, &spiMax, "%u", ImGuiSliderFlags_Logarithmic);
			ImGui::TextDisabled("(Higher = faster convergence per frame, heavier GPU dispatch)");
			min = 2; max = 6;
			ImGui::SliderScalar("Bounces", ImGuiDataType_U32, &settings->NumberOfBounces, &min, &max);
			ImGui::NewLine();

			ImGui::Text("Ray Visualization");
			min = 1; max = 128;
			ImGui::SliderScalarN("Max Rays", ImGuiDataType_U32, &settings->MaxRays, 1, &min, &max);
		}

		// ── Camera (shown for all modes) ───────────────────────────────────────
		ImGui::Text("Camera");
		ImGui::Separator();
		if (ImGui::Checkbox("Right Click to Move Camera", &settings->rightClickToMoveCamera))
		{
			CameraManager::getInstance()->getActiveCamera()->setRightClickToMoveCamera(settings->rightClickToMoveCamera);
		}
		ImGui::Separator();
		ImGui::SliderFloat("FoV", &settings->FieldOfView, UserSettings::FieldOfViewMinValue, UserSettings::FieldOfViewMaxValue, "%.0f");
		ImGui::SliderFloat("Aperture", &settings->Aperture, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat("Focus", &settings->FocusDistance, 0.1f, 20.0f, "%.1f");
		ImGui::NewLine();

		// ── Physics ────────────────────────────────────────────────────────────
		ImGui::Text("Physics");
		ImGui::Separator();
		ImGui::SliderFloat("Physics Timestep", &settings->PhysicsTimestep, 0.01f, 1.0f, "%.3f");
		ImGui::TextDisabled("(Controls how often physics simulation updates)");
		ImGui::NewLine();

		// ── Adaptive Sampling (ray tracing only) ──────────────────────────────
		if (!isGameMode)
		{
			ImGui::Text("Adaptive Sampling");
			ImGui::Separator();
			ImGui::Checkbox("Enable Adaptive Sampling", &settings->EnableAdaptiveSampling);
			if (settings->EnableAdaptiveSampling)
			{
				ImGui::SliderFloat("Variance Threshold", &settings->VarianceThreshold, 0.0001f, 1.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
				uint32_t minSamplesMin = 1, minSamplesMax = 256;
				ImGui::SliderScalar("Min Samples", ImGuiDataType_U32, &settings->MinSamples, &minSamplesMin, &minSamplesMax);
			}
			ImGui::Text("Total Samples Accumulated: %u", settings->MaxNumberOfSamples);
			ImGui::NewLine();
		}

		// ── Game Mode controls ─────────────────────────────────────────────────
		if (isGameMode)
		{
			ImGui::Text("Game Renderer");
			ImGui::Separator();
			ImGui::Text("FPS: %.1f  (%.2f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
			ImGui::NewLine();
			ImGui::SeparatorText("Photorealism");
			ImGui::SliderFloat("Exposure",        &settings->Game.Exposure,        0.1f, 5.0f,  "%.2f");
			ImGui::SliderFloat("Bloom Threshold", &settings->Game.BloomThreshold,  0.5f, 3.0f,  "%.2f");
			ImGui::SliderFloat("Bloom Intensity", &settings->Game.BloomIntensity,  0.0f, 2.0f,  "%.2f");
			ImGui::SliderFloat("SSAO Radius",     &settings->Game.SSAORadius,      0.1f, 2.0f,  "%.2f");
			ImGui::SliderFloat("SSAO Bias",       &settings->Game.SSAOBias,        0.001f, 0.1f,"%.3f");
			ImGui::Checkbox("Enable TAA",         &settings->Game.EnableTAA);
			ImGui::Checkbox("Enable SSR",         &settings->Game.EnableSSR);
			ImGui::Checkbox("Enable SSAO",        &settings->Game.EnableSSAO);
			ImGui::Checkbox("Enable Bloom",       &settings->Game.EnableBloom);
			ImGui::Checkbox("Enable IBL",         &settings->Game.EnableIBL);
			if (settings->Game.EnableIBL)
			{
				ImGui::Checkbox("Use Color in IBL",   &settings->Game.UseColorIBL);
				if (settings->Game.UseColorIBL)
					ImGui::ColorEdit3("IBL Sky Color", glm::value_ptr(settings->Game.IBLSkyColor));
			}
			ImGui::ColorEdit3("Fallback Ambient Color", glm::value_ptr(settings->Game.FallbackAmbientColor));
			ImGui::NewLine();
		}

		// ── Profiler (shown for all modes) ─────────────────────────────────────
		ImGui::Text("Profiler");
		ImGui::Separator();
		ImGui::Checkbox("Show heatmap", &settings->ShowHeatmap);
		ImGui::SliderFloat("Scaling", &settings->HeatmapScale, 0.10f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		ImGui::NewLine();
	}
	ImGui::End();
}
