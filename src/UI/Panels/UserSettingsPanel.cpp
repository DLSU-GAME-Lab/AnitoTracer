#include "UserSettingsPanel.hpp"
#include "../../UserSettings.hpp"
#include "../../Asset/ProjectLoader.hpp"
#include "imgui.h"
#include <iostream>

namespace Diligent {

    UserSettingsPanel::UserSettingsPanel(const std::string& name)
        : BasePanel(name)
    {}

    void UserSettingsPanel::Draw()
    {
        // Do not render if the panel is toggled off
        if (!m_IsVisible) return;

        // Begin the ImGui window with the panel's name and visibility state
        if (ImGui::Begin(m_Name.c_str(), &m_IsVisible))
        {
            auto& projectSettings = ProjectLoader::GetProjectSettings();
            auto& lightingSettings = projectSettings.lighting;
            auto saveProjectSettings = [] {
                if (!ProjectLoader::GetCurrentProjectFile().empty() && !ProjectLoader::SaveProject()) {
                    std::cerr << "Failed to save project settings." << std::endl;
                }
            };

            // ------------------------------------------------------------------
            // Project Lighting Settings Section
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Project Lighting Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PushItemWidth(150.0f);

                ImGui::DragFloat("Shadow Bias", &lightingSettings.shadow_bias, 0.0002f, 0.0f, 0.1f, "%.4f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Epsilon offset applied to ray origin to prevent shadow acne.");
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();

                ImGui::ColorEdit3("Ambient Color", lightingSettings.ambient_color.data());
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Color of the constant ambient contribution.");
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();

                ImGui::DragFloat("Ambient Multiplier", &lightingSettings.ambient_multiplier, 0.01f, 0.0f, 10.0f, "%.2f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Scales the brightness of ambient lighting in the scene.");
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();

                ImGui::PopItemWidth();

                ImGui::Spacing();

                // Convenient Reset Button
                if (ImGui::Button("Reset to Defaults"))
                {
                    lightingSettings = ProjectLightingSettings{};
                    saveProjectSettings();
                }
            }

            // ------------------------------------------------------------------
            // Project Fog Settings Section
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Fog Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& fogSettings = projectSettings.fog;
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", text);
                };
                auto saveAfterEdit = [&] {
                    if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
                };

                ImGui::PushItemWidth(150.0f);

                // Linear Fog
                if (ImGui::Checkbox("Linear Fog", &fogSettings.linear_enabled)) saveProjectSettings();
                tooltip("Blends geometry toward a flat fog color based on distance from the camera.");

                ImGui::BeginDisabled(!fogSettings.linear_enabled);
                ImGui::Indent();

                ImGui::ColorEdit3("Linear Color", fogSettings.linear_color.data());
                saveAfterEdit();

                ImGui::DragFloat("Start Distance", &fogSettings.linear_start, 0.5f, 0.0f, fogSettings.linear_end, "%.1f");
                tooltip("Distance where fog begins.");
                saveAfterEdit();

                ImGui::DragFloat("End Distance", &fogSettings.linear_end, 0.5f, fogSettings.linear_start, 10000.0f, "%.1f");
                tooltip("Distance where fog reaches its maximum opacity.");
                saveAfterEdit();

                ImGui::SliderFloat("Max Opacity", &fogSettings.linear_max_opacity, 0.0f, 1.0f, "%.2f");
                saveAfterEdit();

                ImGui::Unindent();
                ImGui::EndDisabled();

                ImGui::Spacing();

                // Volumetric Fog
                if (ImGui::Checkbox("Volumetric Fog", &fogSettings.volumetric_enabled)) saveProjectSettings();
                tooltip("Ray-marched height fog that scatters light from directional and point lights.");

                ImGui::BeginDisabled(!fogSettings.volumetric_enabled);
                ImGui::Indent();

                ImGui::ColorEdit3("Scattering Color", fogSettings.volumetric_color.data());
                tooltip("Tint applied to light scattered by the fog.");
                saveAfterEdit();

                ImGui::DragFloat("Density", &fogSettings.volumetric_density, 0.001f, 0.0f, 1.0f, "%.3f");
                tooltip("How thick the fog is at its base height.");
                saveAfterEdit();

                ImGui::DragFloat("Base Height", &fogSettings.volumetric_base_height, 0.1f, -1000.0f, 1000.0f, "%.1f");
                tooltip("World Y below which the fog is at full density.");
                saveAfterEdit();

                ImGui::DragFloat("Height Falloff", &fogSettings.volumetric_height_falloff, 0.005f, 0.0f, 5.0f, "%.3f");
                tooltip("How quickly the fog thins out above the base height. 0 = uniform fog.");
                saveAfterEdit();

                ImGui::DragFloat("Max Distance", &fogSettings.volumetric_max_distance, 1.0f, 1.0f, 5000.0f, "%.0f");
                tooltip("Fog is only ray-marched up to this distance from the camera.");
                saveAfterEdit();

                ImGui::SliderFloat("Anisotropy", &fogSettings.volumetric_anisotropy, -0.9f, 0.9f, "%.2f");
                tooltip("Scattering direction. Positive values make the fog glow when looking toward lights.");
                saveAfterEdit();

                ImGui::DragFloat("Light Intensity", &fogSettings.volumetric_light_intensity, 0.01f, 0.0f, 20.0f, "%.2f");
                tooltip("Scales how much scene light the fog scatters.");
                saveAfterEdit();

                ImGui::DragFloat("Ambient Intensity", &fogSettings.volumetric_ambient_intensity, 0.01f, 0.0f, 5.0f, "%.2f");
                tooltip("Unlit brightness of the fog, so it stays visible away from lights.");
                saveAfterEdit();

                ImGui::SliderFloat("Noise Strength", &fogSettings.volumetric_noise_strength, 0.0f, 1.0f, "%.2f");
                tooltip("How patchy the fog is. 0 = smooth.");
                saveAfterEdit();

                ImGui::DragFloat("Noise Scale", &fogSettings.volumetric_noise_scale, 0.001f, 0.001f, 2.0f, "%.3f");
                tooltip("Frequency of the fog patches. Smaller values give larger patches.");
                saveAfterEdit();

                ImGui::DragFloat3("Wind", fogSettings.volumetric_wind.data(), 0.05f);
                tooltip("Velocity the fog patches drift at, in world units per second.");
                saveAfterEdit();

                ImGui::SliderInt("Sample Count", &fogSettings.volumetric_sample_count, 4, 128);
                tooltip("Ray-march steps per pixel. Higher is smoother but slower.");
                saveAfterEdit();

                ImGui::Unindent();
                ImGui::EndDisabled();

                ImGui::Spacing();

                ImGui::BeginDisabled(!fogSettings.linear_enabled && !fogSettings.volumetric_enabled);
                if (ImGui::Checkbox("Apply Fog to Skybox", &fogSettings.apply_to_skybox)) saveProjectSettings();
                tooltip("Also fogs the skybox, treating it as infinitely far away.");
                ImGui::EndDisabled();

                ImGui::PopItemWidth();

                ImGui::Spacing();

                if (ImGui::Button("Reset Fog to Defaults"))
                {
                    fogSettings = ProjectFogSettings{};
                    saveProjectSettings();
                }
            }

            // ------------------------------------------------------------------
            // Project Color Correction Section (game view only)
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Color Correction", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& ccSettings = projectSettings.color_correction;
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", text);
                };
                auto saveAfterEdit = [&] {
                    if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
                };

                ImGui::PushItemWidth(150.0f);

                if (ImGui::Checkbox("Enable Color Correction", &ccSettings.enabled)) saveProjectSettings();
                tooltip("Grades the final game image. Only affects the Game view and release builds, not the Editor view.");

                ImGui::BeginDisabled(!ccSettings.enabled);
                ImGui::Indent();

                ImGui::DragFloat("Exposure", &ccSettings.exposure, 0.01f, -10.0f, 10.0f, "%.2f EV");
                tooltip("Brightness in stops. +1 doubles the brightness, -1 halves it.");
                saveAfterEdit();

                ImGui::SliderFloat("Contrast", &ccSettings.contrast, 0.0f, 2.0f, "%.2f");
                tooltip("Spreads tones away from mid-grey. 1 = unchanged.");
                saveAfterEdit();

                ImGui::SliderFloat("Saturation", &ccSettings.saturation, 0.0f, 2.0f, "%.2f");
                tooltip("0 = greyscale, 1 = unchanged.");
                saveAfterEdit();

                ImGui::SliderFloat("Gamma", &ccSettings.gamma, 0.2f, 3.0f, "%.2f");
                tooltip("Midtone brightness. >1 brightens, <1 darkens. 1 = unchanged.");
                saveAfterEdit();

                ImGui::SliderFloat("Hue Shift", &ccSettings.hue_shift, -180.0f, 180.0f, "%.0f deg");
                tooltip("Rotates every color around the color wheel.");
                saveAfterEdit();

                ImGui::Spacing();
                ImGui::TextDisabled("White Balance");

                ImGui::SliderFloat("Temperature", &ccSettings.temperature, -1.0f, 1.0f, "%.2f");
                tooltip("Negative = cooler (blue), positive = warmer (orange).");
                saveAfterEdit();

                ImGui::SliderFloat("Tint", &ccSettings.tint, -1.0f, 1.0f, "%.2f");
                tooltip("Negative = green, positive = magenta.");
                saveAfterEdit();

                ImGui::ColorEdit3("Color Filter", ccSettings.color_filter.data());
                tooltip("Multiplies the whole image by this color. White = unchanged.");
                saveAfterEdit();

                ImGui::Spacing();
                ImGui::TextDisabled("Shadows / Midtones / Highlights");

                ImGui::ColorEdit3("Shadows", ccSettings.shadows.data());
                tooltip("Tints the darkest parts of the image. Only the hue is used; white = unchanged.");
                saveAfterEdit();

                ImGui::ColorEdit3("Midtones", ccSettings.midtones.data());
                tooltip("Tints the midtones. Only the hue is used; white = unchanged.");
                saveAfterEdit();

                ImGui::ColorEdit3("Highlights", ccSettings.highlights.data());
                tooltip("Tints the brightest parts of the image. Only the hue is used; white = unchanged.");
                saveAfterEdit();

                ImGui::Unindent();
                ImGui::EndDisabled();

                ImGui::PopItemWidth();

                ImGui::Spacing();

                if (ImGui::Button("Reset Color Correction to Defaults"))
                {
                    ccSettings = ProjectColorCorrectionSettings{};
                    saveProjectSettings();
                }
            }

            if (ImGui::CollapsingHeader("Graphics Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enable MSAA (4x)", &UserSettings::GetInstance().GetEnableMSAA());
            }

            if (ImGui::CollapsingHeader("Physics Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Checkbox("Enable Gravity", &projectSettings.physics.gravity_enabled)) {
                    saveProjectSettings();
                }
                ImGui::BeginDisabled(!projectSettings.physics.gravity_enabled);
                ImGui::DragFloat3("Gravity", projectSettings.physics.gravity.data(), 0.1f);
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
                ImGui::EndDisabled();
            }

            if (ImGui::CollapsingHeader("Debug Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Checkbox("Show Physics Collider Wireframes", &projectSettings.debug.show_physics_collider_wireframes)) {
                    saveProjectSettings();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Draws Jolt collider shapes as wireframes in the editor viewport. Has no effect in the game view.");
                }
            }

            // Future settings categories (e.g., Graphics, Audio) can be added here
        }
        ImGui::End();
    }

}