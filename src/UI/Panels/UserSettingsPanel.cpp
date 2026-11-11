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

                ImGui::DragFloat("Sky Light Intensity", &lightingSettings.sky_light_intensity, 0.01f, 0.0f, 10.0f, "%.2f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Hybrid renderer: how strongly the skybox lights the scene.");
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();

                ImGui::DragFloat("Reflection Intensity", &lightingSettings.reflection_intensity, 0.01f, 0.0f, 2.0f, "%.2f");
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Hybrid renderer: strength of skybox reflections on surfaces.");
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
            // HDR Section: exposure and tone mapping of the display pass
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("HDR & Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& hdrSettings = projectSettings.hdr;
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", text);
                };
                auto saveAfterEdit = [&] {
                    if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
                };

                ImGui::TextDisabled("The scene is rendered in HDR. These settings fit it onto the screen.");

                ImGui::PushItemWidth(150.0f);

                // Order matches ProjectHDRSettings::tone_mapper
                const char* toneMapperNames[] = { "Off (Clamp)", "AgX", "Khronos PBR Neutral" };
                if (ImGui::Combo("Tone Mapper", &hdrSettings.tone_mapper, toneMapperNames, IM_ARRAYSIZE(toneMapperNames))) saveProjectSettings();
                tooltip("How light brighter than white is fitted onto the screen.\n"
                    "Off: clipped at white, as it was before HDR.\n"
                    "AgX: film-like. Bright, saturated light fades toward white; softer contrast overall.\n"
                    "Khronos PBR Neutral: shows colors as authored and only rolls off the highlights.\n"
                    "The CrossHatch renderers are never tone mapped.");

                ImGui::DragFloat("Exposure##HDR", &hdrSettings.exposure, 0.01f, -10.0f, 10.0f, "%.2f EV");
                tooltip("Scene brightness in stops, applied before the tone mapper. +1 doubles the light, -1 halves it.");
                saveAfterEdit();

                if (ImGui::Checkbox("Apply to Editor View##HDR", &hdrSettings.apply_to_editor_view)) saveProjectSettings();
                tooltip("Use the exposure and tone mapper in the Editor view too, so it matches the Game view.\nOff shows the Editor view clipped at white.");

                if (ImGui::Checkbox("Dither", &hdrSettings.dither)) saveProjectSettings();
                tooltip("Adds very fine noise to the final image, which hides banding in smooth gradients such as sky and fog.");

                if (hdrSettings.tone_mapper == 1)
                {
                    ImGui::Spacing();
                    ImGui::TextDisabled("AgX Look");

                    // The three looks published with AgX
                    auto setLook = [&](float slopeR, float slopeG, float slopeB, float power, float saturation) {
                        hdrSettings.agx_slope = { slopeR, slopeG, slopeB };
                        hdrSettings.agx_offset = 0.0f;
                        hdrSettings.agx_power = power;
                        hdrSettings.agx_saturation = saturation;
                        saveProjectSettings();
                    };
                    if (ImGui::Button("Base")) setLook(1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
                    tooltip("The plain AgX curve.");
                    ImGui::SameLine();
                    if (ImGui::Button("Golden")) setLook(1.0f, 0.9f, 0.5f, 0.8f, 0.8f);
                    tooltip("Warm and soft.");
                    ImGui::SameLine();
                    if (ImGui::Button("Punchy")) setLook(1.0f, 1.0f, 1.0f, 1.35f, 1.4f);
                    tooltip("More contrast and saturation.");

                    ImGui::DragFloat3("Slope##AgX", hdrSettings.agx_slope.data(), 0.005f, 0.0f, 2.0f, "%.2f");
                    tooltip("Brightness per channel (red, green, blue). Unequal values tint the image.");
                    saveAfterEdit();

                    ImGui::DragFloat("Offset##AgX", &hdrSettings.agx_offset, 0.001f, -0.2f, 0.2f, "%.3f");
                    tooltip("Lifts (positive) or crushes (negative) the darkest tones.");
                    saveAfterEdit();

                    ImGui::DragFloat("Power##AgX", &hdrSettings.agx_power, 0.005f, 0.1f, 3.0f, "%.2f");
                    tooltip("Contrast. Above 1 darkens the midtones for a punchier image, below 1 lifts them.");
                    saveAfterEdit();

                    ImGui::SliderFloat("Saturation##AgX", &hdrSettings.agx_saturation, 0.0f, 2.0f, "%.2f");
                    tooltip("AgX lowers saturation by design; raise this to put some back. 1 = the plain curve.");
                    saveAfterEdit();
                }
                else if (hdrSettings.tone_mapper == 2)
                {
                    ImGui::Spacing();
                    ImGui::TextDisabled("Khronos PBR Neutral");

                    ImGui::SliderFloat("Highlight Start", &hdrSettings.neutral_compression_start, 0.1f, 0.99f, "%.2f");
                    tooltip("Brightness where highlights start to roll off. Darker colors are shown as authored.\nLower gives a longer, softer roll-off. 0.80 is the Khronos default.");
                    saveAfterEdit();

                    ImGui::SliderFloat("Highlight Desaturation", &hdrSettings.neutral_desaturation, 0.0f, 1.0f, "%.2f");
                    tooltip("How quickly very bright colors fade toward white. 0.15 is the Khronos default.");
                    saveAfterEdit();
                }

                ImGui::PopItemWidth();

                ImGui::Spacing();

                if (ImGui::Button("Reset HDR to Defaults"))
                {
                    hdrSettings = ProjectHDRSettings{};
                    saveProjectSettings();
                }
            }

            // ------------------------------------------------------------------
            // Project God Rays Section (game view only)
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("God Rays", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& grSettings = projectSettings.god_rays;
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", text);
                };
                auto saveAfterEdit = [&] {
                    if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
                };

                ImGui::PushItemWidth(150.0f);

                if (ImGui::Checkbox("Enable God Rays", &grSettings.enabled)) saveProjectSettings();
                tooltip("Light shafts streaming from the first directional light. Only affects the Game view and release builds, not the Editor view.");

                ImGui::BeginDisabled(!grSettings.enabled);
                ImGui::Indent();

                ImGui::DragFloat("Intensity##GodRays", &grSettings.intensity, 0.01f, 0.0f, 10.0f, "%.2f");
                tooltip("Overall brightness of the shafts.");
                saveAfterEdit();

                ImGui::SliderFloat("Length", &grSettings.density, 0.0f, 1.0f, "%.2f");
                tooltip("How far toward the sun each pixel gathers light. Higher values give longer shafts.");
                saveAfterEdit();

                ImGui::SliderFloat("Decay", &grSettings.decay, 0.8f, 0.999f, "%.3f");
                tooltip("How quickly the shafts fade along their length. Closer to 1 = longer, softer shafts.");
                saveAfterEdit();

                ImGui::DragFloat("Threshold", &grSettings.threshold, 0.01f, 0.0f, 100.0f, "%.2f");
                tooltip("Only pixels brighter than this emit shafts. Raise it if bright objects near the sun cause unwanted rays.\nThe scene is HDR, so this can go above 1 (white).");
                saveAfterEdit();

                ImGui::DragFloat("Threshold Softness", &grSettings.threshold_knee, 0.005f, 0.01f, 10.0f, "%.2f");
                tooltip("Brightness range above the threshold over which pixels fade in to full strength. Lower gives a harder cut-off.");
                saveAfterEdit();

                ImGui::DragFloat("Max Brightness", &grSettings.max_brightness, 0.05f, 0.1f, 1000.0f, "%.2f");
                tooltip("Caps how bright an emitting pixel counts. Keeps a few very bright pixels (the sun's disk, strong emissives)\nfrom dominating the shafts or flickering.");
                saveAfterEdit();

                ImGui::SliderFloat("Sun Radius", &grSettings.sun_radius, 0.01f, 2.0f, "%.2f");
                tooltip("Size of the region around the sun that emits shafts, as a fraction of the screen height.");
                saveAfterEdit();

                ImGui::SliderInt("Sample Count##GodRays", &grSettings.sample_count, 8, 128);
                tooltip("Samples per pixel along each shaft. Higher is smoother but slower.");
                saveAfterEdit();

                if (ImGui::Checkbox("Use Light Color", &grSettings.use_light_color)) saveProjectSettings();
                tooltip("Multiply the tint by the directional light's color and intensity.");

                ImGui::ColorEdit3("Tint##GodRays", grSettings.tint.data());
                tooltip("Color of the shafts.");
                saveAfterEdit();

                ImGui::Unindent();
                ImGui::EndDisabled();

                ImGui::PopItemWidth();

                ImGui::Spacing();

                if (ImGui::Button("Reset God Rays to Defaults"))
                {
                    grSettings = ProjectGodRaysSettings{};
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
                tooltip("Grades the final game image. Only affects the Game view and release builds, not the Editor view.\nExposure, white balance and the color filter act before the tone mapper; the rest acts on the tone mapped image.");

                ImGui::BeginDisabled(!ccSettings.enabled);
                ImGui::Indent();

                ImGui::DragFloat("Exposure", &ccSettings.exposure, 0.01f, -10.0f, 10.0f, "%.2f EV");
                tooltip("Brightness in stops, on top of the HDR exposure. +1 doubles the brightness, -1 halves it.\nApplied to the scene light, before the tone mapper.");
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

            // ------------------------------------------------------------------
            // Cross Hatching Section (CrossHatch renderers only)
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Cross Hatching"))
            {
                auto& chSettings = projectSettings.crosshatch;
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", text);
                };
                auto saveAfterEdit = [&] {
                    if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
                };

                ImGui::TextDisabled("Used by the CrossHatch renderers (Rendering menu), in the Game view.");

                ImGui::PushItemWidth(150.0f);

                // Order matches ProjectCrossHatchSettings::mode
                const char* modeNames[] = { "Noise Hatching", "Toon Texture Hatching", "Toon + Noise Hatching" };
                if (ImGui::Combo("Style", &chSettings.mode, modeNames, IM_ARRAYSIZE(modeNames))) saveProjectSettings();
                tooltip("Noise: world-space pencil strokes, denser where darker.\nToon Texture: cel-shaded paper with hand-drawn strokes along the mesh UVs.\nToon + Noise: solid ink shadows, hatched midtones, clean highlights.");

                ImGui::ColorEdit3("Ink Color", chSettings.ink_color.data());
                tooltip("Color of the strokes, outlines and inner edges.");
                saveAfterEdit();

                ImGui::SliderFloat("Hatch Opacity", &chSettings.hatch_opacity, 0.0f, 1.0f, "%.2f");
                tooltip("Blends between plain lighting (0) and full hatching (1).");
                saveAfterEdit();

                if (ImGui::Checkbox("Inner Edges", &chSettings.inner_edges)) saveProjectSettings();
                tooltip("Ink along creases where the surface bends sharply.");

                // Noise strokes, used by Noise and Toon + Noise
                if (chSettings.mode != 1)
                {
                    ImGui::Spacing();
                    ImGui::TextDisabled("Noise Strokes");

                    ImGui::SliderInt("Noise Texture", &chSettings.noise_texture, 1, 11);
                    tooltip("Noise the strokes are drawn from (Assets/CrossHatch/Noise). Each gives a different stroke character.");
                    saveAfterEdit();

                    ImGui::DragFloat("Scale##HatchLit", &chSettings.lit_scale, 0.005f, 0.01f, 10.0f, "%.3f");
                    tooltip("Stroke frequency in world space. Higher = finer strokes.");
                    saveAfterEdit();

                    ImGui::DragFloat("Density##HatchLit", &chSettings.lit_density, 0.01f, 0.0f, 10.0f, "%.2f");
                    tooltip("How quickly strokes stack up as the surface darkens.");
                    saveAfterEdit();

                    ImGui::DragFloat("Weight##HatchLit", &chSettings.lit_thickness, 0.01f, 0.0f, 10.0f, "%.2f");
                    tooltip("Stroke weight. Multiplies with Density.");
                    saveAfterEdit();

                    ImGui::SliderAngle("Angle##HatchLit", &chSettings.lit_angle, -180.0f, 180.0f);
                    saveAfterEdit();

                    ImGui::SliderFloat("Smoothness##HatchLit", &chSettings.line_smoothness, 0.001f, 0.5f, "%.3f");
                    tooltip("Softness of the stroke edges.");
                    saveAfterEdit();

                    ImGui::TextDisabled("Detail Layer");

                    ImGui::DragFloat("Scale##HatchDetail", &chSettings.detail_scale, 0.005f, 0.01f, 10.0f, "%.3f");
                    saveAfterEdit();

                    ImGui::DragFloat("Density##HatchDetail", &chSettings.detail_density, 0.01f, 0.0f, 10.0f, "%.2f");
                    tooltip("Strength of the constant stroke layer drawn everywhere. 0 = none.");
                    saveAfterEdit();

                    ImGui::DragFloat("Weight##HatchDetail", &chSettings.detail_thickness, 0.1f, 0.0f, 40.0f, "%.1f");
                    saveAfterEdit();

                    ImGui::SliderAngle("Angle##HatchDetail", &chSettings.detail_angle, -180.0f, 180.0f);
                    saveAfterEdit();
                }

                // Shadow strokes, Noise only
                if (chSettings.mode == 0)
                {
                    ImGui::Spacing();
                    ImGui::TextDisabled("Shadow Strokes");

                    ImGui::DragFloat("Intensity##HatchShadow", &chSettings.shadow_intensity, 0.01f, 0.0f, 1.0f, "%.2f");
                    tooltip("Strength of the extra strokes inside the key light's shadow. Cast shadows need the ray-traced CrossHatch renderer.");
                    saveAfterEdit();

                    ImGui::DragFloat("Scale##HatchShadow", &chSettings.shadow_scale, 0.005f, 0.01f, 10.0f, "%.3f");
                    saveAfterEdit();

                    ImGui::DragFloat("Density##HatchShadow", &chSettings.shadow_density, 0.01f, 0.0f, 10.0f, "%.2f");
                    saveAfterEdit();

                    ImGui::DragFloat("Weight##HatchShadow", &chSettings.shadow_thickness, 0.01f, 0.0f, 10.0f, "%.2f");
                    saveAfterEdit();

                    ImGui::SliderAngle("Angle##HatchShadow", &chSettings.shadow_angle, -180.0f, 180.0f);
                    saveAfterEdit();

                    ImGui::SliderFloat("Smoothness##HatchShadow", &chSettings.shadow_smoothness, 0.001f, 0.5f, "%.3f");
                    saveAfterEdit();

                    ImGui::SliderFloat("Core Bias", &chSettings.shadow_core_bias, 0.01f, 2.0f, "%.2f");
                    tooltip("Below 1 keeps the strokes dense right up to the edge of the shadow.");
                    saveAfterEdit();
                }

                // Stroke texture, Toon Texture only
                if (chSettings.mode == 1)
                {
                    ImGui::Spacing();
                    ImGui::TextDisabled("Stroke Texture");

                    ImGui::DragFloat("Scale##HatchTex", &chSettings.texture_scale, 0.05f, 0.1f, 64.0f, "%.2f");
                    tooltip("Stroke texture repeats per UV unit.");
                    saveAfterEdit();

                    ImGui::SliderAngle("Rotation##HatchTex", &chSettings.texture_rotation, -180.0f, 180.0f);
                    saveAfterEdit();

                    ImGui::SliderFloat("Thickness##HatchTex", &chSettings.texture_thickness, -0.5f, 0.5f, "%.2f");
                    tooltip("Fattens (positive) or thins (negative) the strokes.");
                    saveAfterEdit();

                    ImGui::SliderFloat("Cross Strokes", &chSettings.texture_priority, 0.0f, 1.0f, "%.2f");
                    tooltip("Weight of the second stroke direction. Lower = cross strokes only appear in darker areas.");
                    saveAfterEdit();

                    ImGui::SliderFloat("Light Bias", &chSettings.texture_light_bias, -1.0f, 1.0f, "%.2f");
                    tooltip("Brightens (positive) or darkens (negative) the light the strokes respond to.");
                    saveAfterEdit();

                    ImGui::SliderFloat("Black Threshold", &chSettings.texture_black_threshold, 0.0f, 1.0f, "%.2f");
                    tooltip("Light level below which the surface turns solid ink.");
                    saveAfterEdit();

                    ImGui::SliderFloat("Smoothness##HatchTex", &chSettings.texture_smoothness, 0.001f, 0.5f, "%.3f");
                    tooltip("Softness of the stroke edges.");
                    saveAfterEdit();

                    if (ImGui::Checkbox("Specular Highlight", &chSettings.specular_enabled)) saveProjectSettings();
                    tooltip("Paints the brightest areas white, like a comic highlight.");

                    ImGui::BeginDisabled(!chSettings.specular_enabled);
                    ImGui::SliderFloat("Specular Threshold", &chSettings.specular_threshold, 0.0f, 2.0f, "%.2f");
                    tooltip("Light level above which the highlight appears. Lower = wider highlight.");
                    saveAfterEdit();
                    ImGui::EndDisabled();
                }

                ImGui::Spacing();
                ImGui::TextDisabled("Outline");

                if (ImGui::Checkbox("Enable Outline", &chSettings.outline_enabled)) saveProjectSettings();
                tooltip("Ink outline around each object's silhouette (inverted hull).");

                ImGui::BeginDisabled(!chSettings.outline_enabled);
                ImGui::DragFloat("Width##Outline", &chSettings.outline_width, 0.0001f, 0.0f, 1.0f, "%.4f");
                tooltip(chSettings.outline_constant_screen_width
                    ? "Outline width as a fraction of the distance to the camera."
                    : "Outline width in world units.");
                saveAfterEdit();

                if (ImGui::Checkbox("Constant Screen Width", &chSettings.outline_constant_screen_width)) saveProjectSettings();
                tooltip("Keeps the outline the same thickness on screen at any distance.");

                if (ImGui::Checkbox("Extrude From Origin", &chSettings.outline_from_origin)) saveProjectSettings();
                tooltip("Grows the outline away from the object's origin instead of along its normals.\nCloses gaps on hard-edged meshes, but needs the origin near the mesh's center.");
                ImGui::EndDisabled();

                ImGui::PopItemWidth();

                ImGui::Spacing();

                if (ImGui::Button("Reset Cross Hatching to Defaults"))
                {
                    chSettings = ProjectCrossHatchSettings{};
                    saveProjectSettings();
                }
            }

            if (ImGui::CollapsingHeader("Graphics Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Enable MSAA (4x)", &UserSettings::GetInstance().GetEnableMSAA());

                ImGui::PushItemWidth(150.0f);
                int& resolutionIndex = UserSettings::GetInstance().GetResolutionIndex();
                const bool resolutionComboOpen = ImGui::BeginCombo("Resolution", UserSettings::GetInstance().GetResolution().Name);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Resolution the Game and Editor views are rendered at.");
                }
                if (resolutionComboOpen)
                {
                    for (int i = 0; i < kRenderResolutionCount; ++i)
                    {
                        const RenderResolution& resolution = kRenderResolutions[i];
                        const std::string label = std::string(resolution.Name) + " (" + std::to_string(resolution.Width) + "x" + std::to_string(resolution.Height) + ")";
                        if (ImGui::Selectable(label.c_str(), resolutionIndex == i)) resolutionIndex = i;
                    }
                    ImGui::EndCombo();
                }

                // Order matches ViewportScaleMode
                const char* scaleModeNames[] = { "Letterbox", "Stretch" };
                int scaleMode = static_cast<int>(UserSettings::GetInstance().GetViewportScaleMode());
                if (ImGui::Combo("Viewport Scaling", &scaleMode, scaleModeNames, IM_ARRAYSIZE(scaleModeNames)))
                {
                    UserSettings::GetInstance().GetViewportScaleMode() = static_cast<ViewportScaleMode>(scaleMode);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Letterbox keeps the resolution's aspect ratio with bars around the image.\nStretch fills the whole Game and Editor panels, distorting the image.");
                }

                ImGui::Spacing();

                // Temporal anti-aliasing
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", text);
                };
                TAASettings& taaSettings = UserSettings::GetInstance().GetTAASettings();

                ImGui::Checkbox("Enable TAA", &taaSettings.Enabled);
                tooltip("Temporal anti-aliasing: smooths jagged edges by jittering the camera slightly every frame and blending the frames together.\nWorks with every renderer, including the deferred ones that can't use MSAA.");

                ImGui::BeginDisabled(!taaSettings.Enabled);
                ImGui::Indent();

                ImGui::Checkbox("Apply to Editor View", &taaSettings.ApplyToEditorView);
                tooltip("Also anti-alias the Editor view. The Game view and release builds always use TAA while it is enabled.");

                ImGui::SliderFloat("History Blend", &taaSettings.HistoryBlend, 0.5f, 0.98f, "%.2f");
                tooltip("How much of the previous frames is kept each frame. Higher is smoother and more stable, but smears more during fast motion.");

                ImGui::SliderFloat("Jitter Spread", &taaSettings.JitterSpread, 0.0f, 2.0f, "%.2f px");
                tooltip("Size of the per-frame camera offset. 1 covers a whole pixel; lower is sharper but smooths edges less. 0 turns off the jitter.");

                ImGui::SliderFloat("Sharpness##TAA", &taaSettings.Sharpness, 0.0f, 1.0f, "%.2f");
                tooltip("Sharpens the final image to offset the slight softness TAA adds. 0 = off.");

                if (ImGui::Button("Reset TAA to Defaults"))
                {
                    taaSettings = TAASettings{};
                }

                ImGui::Unindent();
                ImGui::EndDisabled();

                ImGui::PopItemWidth();
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

            // ------------------------------------------------------------------
            // Audio Settings Section
            // ------------------------------------------------------------------

            if (ImGui::CollapsingHeader("Audio Settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::SliderFloat("Master Volume", &projectSettings.audio.master_volume, AudioManager::MinVolume, 
                    AudioManager::MaxVolume, "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
                {
                    AudioManager::GetInstance().SetMasterGroupVolume(projectSettings.audio.master_volume);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();

                if (ImGui::SliderFloat("Background Music Volume", &projectSettings.audio.bgm_volume, AudioManager::MinVolume, 
                    AudioManager::MaxVolume, "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
                {
                    AudioManager::GetInstance().SetAudioGroupVolume(AudioManager::AudioGroupType::BGM, 
                        projectSettings.audio.bgm_volume);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();

                if (ImGui::SliderFloat("Sound Effects Volume", &projectSettings.audio.sfx_volume, AudioManager::MinVolume, 
                    AudioManager::MaxVolume, "%.0f%%", ImGuiSliderFlags_AlwaysClamp))
                {
                    AudioManager::GetInstance().SetAudioGroupVolume(AudioManager::AudioGroupType::SFX, 
                        projectSettings.audio.sfx_volume);
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
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