#include "UserSettingsPanel.hpp"
#include "../../UserSettings.hpp"
#include "../../Asset/ProjectLoader.hpp"
#include "RendererManager.hpp"
#include "imgui.h"
#include <filesystem>
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
                    ImGui::SetTooltip("Color of the constant ambient contribution.\nNot added where per-pixel GI is in use: that gathers the real indirect light instead.");

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
                    ImGui::SetTooltip("Hybrid renderer: how strongly the sky lights the scene, on top of the Skybox intensity.");
                }
                if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();

                ImGui::PopItemWidth();

                ImGui::Spacing();

                // Convenient Reset Button
                if (ImGui::Button("Reset to Defaults"))
                {
                    // The reflection intensity is edited (and reset) in the Reflections section below
                    const float reflectionIntensity = lightingSettings.reflection_intensity;
                    lightingSettings = ProjectLightingSettings{};
                    lightingSettings.reflection_intensity = reflectionIntensity;
                    saveProjectSettings();
                }
            }

            // ------------------------------------------------------------------
            // Reflections Section (Hybrid and Deferred Hybrid renderers)
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Reflections", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& reflectionSettings = projectSettings.reflections;
                // Disabled controls still explain themselves
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", text);
                };
                auto saveAfterEdit = [&] {
                    if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
                };

                const bool rayTracingSupported = ::RendererManager::GetInstance().IsRayTracingSupported();
                const bool objectsSupported = ::RendererManager::GetInstance().IsReflectionShadingSupported();

                if (!rayTracingSupported)
                {
                    ImGui::TextDisabled("This device has no hardware ray tracing.");
                }

                // Everything that traces rays; greyed out as a group where rays can't be traced at all
                ImGui::BeginDisabled(!rayTracingSupported);

                if (ImGui::Checkbox("Ray-Traced Reflections", &reflectionSettings.enabled)) saveProjectSettings();
                tooltip("Hybrid renderers: trace a ray along each reflection to find what is really there.\n"
                    "Off: surfaces reflect the sky only, even where an object is in the way. Fastest.");

                ImGui::BeginDisabled(!reflectionSettings.enabled);
                ImGui::Indent();

                ImGui::BeginDisabled(!objectsSupported);
                if (ImGui::Checkbox("Show Reflected Objects", &reflectionSettings.shade_hits)) saveProjectSettings();
                tooltip(objectsSupported
                    ? "Objects appear in reflections, lit and textured.\nOff: an object only blocks the sky and leaves a dim patch. Faster."
                    : "Not available on this device: it can't pick textures per pixel, which shading reflected objects needs.");

                ImGui::BeginDisabled(!reflectionSettings.shade_hits);
                ImGui::Indent();

                if (ImGui::Checkbox("Shadows in Reflections", &reflectionSettings.hit_shadows)) saveProjectSettings();
                tooltip("Reflected objects keep their shadows.\nCosts one more ray per light for every reflecting pixel. Off: they are lit as if nothing shadowed them.");

                if (ImGui::Checkbox("Blur on Rough Surfaces", &reflectionSettings.glossy)) saveProjectSettings();
                tooltip("Rough surfaces blur what they reflect instead of showing it mirror sharp.\n"
                    "Deferred Hybrid cleans the blur up with its denoiser (see Denoising).\n"
                    "Without the denoiser it is built up over several frames, so it only applies while TAA is on, and stays a little grainy.");

                ImGui::Unindent();
                ImGui::EndDisabled();
                ImGui::EndDisabled();

                ImGui::PushItemWidth(150.0f);

                ImGui::SliderFloat("Max Roughness##Reflections", &reflectionSettings.max_roughness, 0.05f, 1.0f, "%.2f");
                tooltip("Surfaces rougher than this reflect the sky only, without tracing a ray.\nRays fade out from half this value. Lower is faster.");
                saveAfterEdit();

                ImGui::DragFloat("Ray Distance##Reflections", &reflectionSettings.max_distance, 1.0f, 1.0f, 10000.0f, "%.0f");
                tooltip("How far a reflection looks for objects, in world units. Anything farther away shows as sky.");
                saveAfterEdit();

                ImGui::PopItemWidth();

                ImGui::Unindent();
                ImGui::EndDisabled();
                ImGui::EndDisabled();

                ImGui::PushItemWidth(150.0f);
                ImGui::DragFloat("Intensity##Reflections", &lightingSettings.reflection_intensity, 0.01f, 0.0f, 2.0f, "%.2f");
                tooltip("Hybrid renderers: strength of all reflections, the sky's included. 0 removes them.");
                saveAfterEdit();
                ImGui::PopItemWidth();

                ImGui::Spacing();

                if (ImGui::Button("Reset Reflections to Defaults"))
                {
                    reflectionSettings = ProjectReflectionSettings{};
                    lightingSettings.reflection_intensity = ProjectLightingSettings{}.reflection_intensity;
                    saveProjectSettings();
                }
            }

            // ------------------------------------------------------------------
            // Global Illumination Section (Hybrid and Deferred Hybrid renderers)
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Global Illumination", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& giSettings = projectSettings.gi;
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", text);
                };
                auto saveAfterEdit = [&] {
                    if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
                };

                const bool rayTracingSupported = ::RendererManager::GetInstance().IsRayTracingSupported();
                // Per-pixel GI shades what its rays hit, which needs the same device support as reflections of objects,
                // and it reads the G-buffer, which only Deferred Hybrid has
                const bool perPixelSupported = ::RendererManager::GetInstance().IsReflectionShadingSupported();
                const bool deferredHybrid = UserSettings::GetInstance().GetRendererType() == DEFERRED_HYBRID;

                if (!rayTracingSupported)
                {
                    ImGui::TextDisabled("This device has no hardware ray tracing.");
                }

                ImGui::BeginDisabled(!rayTracingSupported);
                ImGui::PushItemWidth(170.0f);

                // Order matches ProjectGIMethod. Where per-pixel can't run, the box shows what is used instead.
                const char* giMethodNames[] = { "Sky + AO", "Per-pixel, one bounce" };
                int shownMethod = perPixelSupported ? (giSettings.method == GI_METHOD_PER_PIXEL ? GI_METHOD_PER_PIXEL : GI_METHOD_SKY_AO) : GI_METHOD_SKY_AO;
                ImGui::BeginDisabled(!perPixelSupported);
                if (ImGui::Combo("GI Method", &shownMethod, giMethodNames, IM_ARRAYSIZE(giMethodNames)))
                {
                    giSettings.method = shownMethod;
                    saveProjectSettings();
                }
                ImGui::EndDisabled();
                tooltip(perPixelSupported
                    ? "How surfaces are lit by the sky and by each other.\n"
                      "Sky + AO: flat ambient color plus sky light, dimmed in corners. No noise, but no color bleeding, and rooms are lit by sky they can't see.\n"
                      "Per-pixel, one bounce: a ray per pixel gathers the light that really arrives. Deferred Hybrid renderer only."
                    : "Per-pixel GI is not available on this device: it can't pick textures per pixel, which shading what the rays hit needs.");

                const bool perPixelChosen = perPixelSupported && giSettings.method == GI_METHOD_PER_PIXEL;
                if (rayTracingSupported && perPixelChosen && !deferredHybrid)
                {
                    ImGui::TextDisabled("In use now: Sky + AO. Per-pixel needs the Deferred Hybrid renderer.");
                }

                // Sky + AO's own setting; the ones after it belong to Per-pixel
                ImGui::BeginDisabled(perPixelChosen);
                if (ImGui::Checkbox("Half-Resolution Occlusion##GI", &giSettings.ao_half_resolution)) saveProjectSettings();
                tooltip("Sky + AO in the Deferred Hybrid renderer: trace and filter the ambient occlusion for every 2x2 pixels and scale the result up.\n"
                    "A quarter of the rays and of the filtering, for slightly softer shading in corners and under objects.\n"
                    "Off: trace for every pixel. For comparing the two.");
                ImGui::EndDisabled();

                ImGui::BeginDisabled(!perPixelChosen);

                ImGui::DragFloat("Intensity##GI", &giSettings.intensity, 0.01f, 0.0f, 10.0f, "%.2f");
                tooltip("Strength of the gathered light. 1 is physically right.");
                saveAfterEdit();

                ImGui::DragFloat("Ray Distance##GI", &giSettings.max_distance, 1.0f, 1.0f, 10000.0f, "%.0f");
                tooltip("How far a ray looks for surfaces, in world units. Anything farther away counts as sky.");
                saveAfterEdit();

                if (ImGui::Checkbox("Half Resolution##GI", &giSettings.half_resolution)) saveProjectSettings();
                tooltip("Trace for every 2x2 pixels and scale the result up.\nA quarter of the rays for a slightly softer result. Off: trace for every pixel.");

                ImGui::SliderInt("Rays per Pixel##GI", &giSettings.rays_per_pixel, 1, 8);
                tooltip("Rays traced per frame for each traced pixel.\nMore rays give a cleaner, steadier result, most visibly where a room is lit by a small bright area. The cost grows with the count.");
                saveAfterEdit();

                if (ImGui::Checkbox("Shadows on Bounce Light", &giSettings.hit_shadows)) saveProjectSettings();
                tooltip("Surfaces only bounce the light that reaches them.\nCosts one more ray per light. Off: light bounces off surfaces that are in shadow too.");

                if (ImGui::Checkbox("Further Bounces", &giSettings.more_bounces)) saveProjectSettings();
                tooltip("Light keeps bouncing from surface to surface, one more bounce every frame, by reusing the last frame's result.\n"
                    "Fills in rooms lit through a small opening. Only surfaces on screen pass light on. Off: light bounces exactly once.");

                if (ImGui::Checkbox("Sky Light on Bounces", &giSettings.sky_bounce)) saveProjectSettings();
                tooltip("Surfaces that bounce light are also lit by the sky, where one more ray reaches it.\nSoftens shaded outdoor areas. Used where Further Bounces has nothing to reuse, or is off.");

                // These two steer the denoiser, so they do nothing while it is off for indirect light
                ImGui::BeginDisabled(!(projectSettings.denoise.enabled && projectSettings.denoise.indirect));

                ImGui::SliderFloat("Smoothing over Time##GI", &giSettings.history_blend, 0.0f, 0.98f, "%.2f");
                tooltip("How much of the earlier frames the denoiser keeps every frame.\nHigher is smoother, but lighting changes and moving objects take longer to catch up.\n"
                    "Needs Denoising on for indirect light.");
                saveAfterEdit();

                ImGui::SliderInt("Blur Passes##GI", &giSettings.filter_passes, 0, 5);
                tooltip("The denoiser's blur passes, which hide the remaining noise. Each doubles the blur's reach.\n0 shows the light as gathered over time.\n"
                    "Needs Denoising on for indirect light.");
                saveAfterEdit();

                ImGui::EndDisabled();

                ImGui::EndDisabled();

                ImGui::PopItemWidth();
                ImGui::EndDisabled();

                ImGui::Spacing();

                if (ImGui::Button("Reset Global Illumination to Defaults"))
                {
                    giSettings = ProjectGISettings{};
                    saveProjectSettings();
                }
            }

            // ------------------------------------------------------------------
            // Denoising Section (Deferred Hybrid renderer)
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Denoising", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& denoiseSettings = projectSettings.denoise;
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", text);
                };

                const bool rayTracingSupported = ::RendererManager::GetInstance().IsRayTracingSupported();
                const bool deferredHybrid = UserSettings::GetInstance().GetRendererType() == DEFERRED_HYBRID;

                if (!rayTracingSupported)
                {
                    ImGui::TextDisabled("This device has no hardware ray tracing.");
                }
                else if (!deferredHybrid)
                {
                    ImGui::TextDisabled("Only the Deferred Hybrid renderer denoises its rays.");
                }

                ImGui::BeginDisabled(!rayTracingSupported);

                // The one switch for comparison tests: off skips every filter below
                if (ImGui::Checkbox("Denoise Ray-Traced Light", &denoiseSettings.enabled)) saveProjectSettings();
                tooltip("Deferred Hybrid renderer: every pixel traces one ray per light, one for its reflection and a few for indirect light.\n"
                    "The filters average those rays over frames and blur them along surfaces.\n"
                    "Off: every filter is skipped and the same rays are shown as traced, noise included. For comparing.");

                ImGui::BeginDisabled(!denoiseSettings.enabled);
                ImGui::Indent();

                if (ImGui::Checkbox("Shadows##Denoise", &denoiseSettings.shadows)) saveProjectSettings();
                tooltip("Filter the shadows. Off: soft shadow edges show the grain of one ray per light.");

                if (ImGui::Checkbox("Reflections##Denoise", &denoiseSettings.reflections)) saveProjectSettings();
                tooltip("Filter the reflections. Off: rough surfaces show the grain of one ray per pixel.");

                if (ImGui::Checkbox("Indirect Light##Denoise", &denoiseSettings.indirect)) saveProjectSettings();
                tooltip("Filter the indirect light: per-pixel GI, or the ambient occlusion of Sky + AO.\n"
                    "Off: it shows the grain of the few rays each pixel traces.");

                ImGui::Unindent();
                ImGui::EndDisabled();
                ImGui::EndDisabled();

                ImGui::Spacing();

                if (ImGui::Button("Reset Denoising to Defaults"))
                {
                    denoiseSettings = ProjectDenoiseSettings{};
                    saveProjectSettings();
                }
            }

            // ------------------------------------------------------------------
            // Transparency Section (Hybrid and Deferred Hybrid renderers)
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Transparency", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& transparencySettings = projectSettings.transparency;
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) ImGui::SetTooltip("%s", text);
                };

                const bool rayTracingSupported = ::RendererManager::GetInstance().IsRayTracingSupported();
                // Telling a cutout's gaps from its solid parts, and what glass lets through, means reading the scene's
                // materials from a ray: the same device support as shading the objects a reflection shows
                const bool sceneRaysSupported = ::RendererManager::GetInstance().IsReflectionShadingSupported();
                const PipelineType rendererType = UserSettings::GetInstance().GetRendererType();
                const bool rayTracedRenderer = rendererType == HYBRID || rendererType == DEFERRED_HYBRID;

                if (!rayTracingSupported)
                {
                    ImGui::TextDisabled("This device has no hardware ray tracing.");
                }
                else if (!sceneRaysSupported)
                {
                    ImGui::TextDisabled("This device can't read materials from a ray: cutouts and glass are solid to rays.");
                }
                else if (!rayTracedRenderer)
                {
                    ImGui::TextDisabled("Only the Hybrid and Deferred Hybrid renderers draw glass and trace cutouts.");
                }

                ImGui::BeginDisabled(!rayTracingSupported);

                // The one switch for comparison tests: off is how the renderers behaved before
                if (ImGui::Checkbox("Cutouts and Glass", &transparencySettings.enabled)) saveProjectSettings();
                tooltip("Cutout materials (fences, leaves) cast shadows shaped like their texture and show in reflections with their holes.\n"
                    "Blended materials are drawn as glass: tinted, refracting, reflecting the scene and casting a tinted shadow.\n"
                    "Off: every triangle blocks every ray, and blended materials are not drawn. For comparing.");

                ImGui::BeginDisabled(!transparencySettings.enabled);
                ImGui::Indent();

                if (ImGui::Checkbox("Cutouts in Rays##Transparency", &transparencySettings.cutout_rays)) saveProjectSettings();
                tooltip("Shadows, reflections and bounced light see the holes in cutout materials.\n"
                    "Off: a cutout's whole card is solid to rays, though it is still drawn with its holes.");

                if (ImGui::Checkbox("Draw Glass##Transparency", &transparencySettings.glass)) saveProjectSettings();
                tooltip("Draw blended materials over the scene, farthest first.\n"
                    "Off: they are not drawn, and rays go through them.");

                ImGui::BeginDisabled(!transparencySettings.glass);
                ImGui::Indent();

                if (ImGui::Checkbox("Tinted Shadows##Transparency", &transparencySettings.tinted_shadows)) saveProjectSettings();
                tooltip("Light that passes through glass takes its color, as much as the glass is opaque.\n"
                    "Deferred Hybrid carries the color for the first four lights; any others only lose brightness.\n"
                    "Off: glass casts a solid shadow.");

                if (ImGui::Checkbox("Refraction##Transparency", &transparencySettings.refraction)) saveProjectSettings();
                tooltip("Glass bends what is behind it, using the material's index of refraction, and tints it.\n"
                    "A closed shape (a ball, a slab) bends the view in and out again; a single sheet shows it unbent.\n"
                    "Off: plain alpha blending, which shows what is behind the glass where it is.");

                ImGui::Unindent();
                ImGui::EndDisabled();

                ImGui::Unindent();
                ImGui::EndDisabled();
                ImGui::EndDisabled();

                ImGui::Spacing();

                if (ImGui::Button("Reset Transparency to Defaults"))
                {
                    transparencySettings = ProjectTransparencySettings{};
                    saveProjectSettings();
                }
            }


            // ------------------------------------------------------------------
            // Skybox Section
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Skybox", ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto& skySettings = projectSettings.sky;
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", text);
                };
                auto saveAfterEdit = [&] {
                    if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
                };

                ImGui::PushItemWidth(150.0f);

                // Order matches ProjectSkyMode
                const char* skyModeNames[] = { "Skybox Texture", "Solid Color" };
                if (ImGui::Combo("Sky Type", &skySettings.mode, skyModeNames, IM_ARRAYSIZE(skyModeNames))) saveProjectSettings();
                tooltip("What is drawn behind the scene.\nIn the ray-traced renderers the sky also lights the scene and shows up in reflections.");

                if (skySettings.mode == SKY_MODE_TEXTURE)
                {
                    const bool usesDefault = skySettings.texture_path.empty();
                    const std::string skyFileName = usesDefault
                        ? std::string("Default skybox")
                        : std::filesystem::path(skySettings.texture_path).filename().string();
                    ImGui::Text("File: %s", skyFileName.c_str());
                    if (!usesDefault && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", skySettings.texture_path.c_str());

                    if (ImGui::Button("Browse...##Sky"))
                    {
                        const std::string picked = gbe::FileDialogue::GetFilePath(gbe::FileDialogue::OPEN, kSkyTextureExtensions);
                        if (!picked.empty())
                        {
                            // Files inside the project are stored relative to it, so the project can be moved or shared
                            std::filesystem::path skyPath(picked);
                            const std::filesystem::path projectDir = ProjectLoader::GetCurrentProjectDir();
                            if (!projectDir.empty())
                            {
                                const std::filesystem::path relative = skyPath.lexically_relative(projectDir);
                                if (!relative.empty() && *relative.begin() != "..") skyPath = relative;
                            }
                            skySettings.texture_path = skyPath.generic_string();
                            saveProjectSettings();
                        }
                    }
                    tooltip("Pick a cubemap (.dds, .ktx) or a 360 degree panorama (.hdr, .png, .jpg, .tga).\n.hdr files keep the sky's real brightness, which gives the most convincing lighting.");

                    ImGui::SameLine();
                    ImGui::BeginDisabled(usesDefault);
                    if (ImGui::Button("Use Default##Sky"))
                    {
                        skySettings.texture_path.clear();
                        saveProjectSettings();
                    }
                    ImGui::EndDisabled();

                    ImGui::SliderFloat("Rotation##Sky", &skySettings.rotation, -180.0f, 180.0f, "%.0f deg");
                    tooltip("Turns the sky around the up axis, for example to move its sun.");
                    saveAfterEdit();
                }
                else
                {
                    ImGui::ColorEdit3("Sky Color", skySettings.color.data());
                    tooltip("The whole sky is this one color.");
                    saveAfterEdit();
                }

                ImGui::DragFloat("Intensity##Sky", &skySettings.intensity, 0.01f, 0.0f, 1000.0f, "%.2f");
                tooltip("Brightness of the sky, and of the light it casts in the ray-traced renderers.");
                saveAfterEdit();

                ImGui::PopItemWidth();

                ImGui::Spacing();

                if (ImGui::Button("Reset Skybox to Defaults"))
                {
                    skySettings = ProjectSkySettings{};
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
                    "The CrossHatch and Anime renderers are never tone mapped.");

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

            // ------------------------------------------------------------------
            // Anime Shading Section (Anime renderer only)
            // ------------------------------------------------------------------
            if (ImGui::CollapsingHeader("Anime Shading"))
            {
                auto& animeSettings = projectSettings.anime;
                auto tooltip = [](const char* text) {
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", text);
                };
                auto saveAfterEdit = [&] {
                    if (ImGui::IsItemDeactivatedAfterEdit()) saveProjectSettings();
                };

                ImGui::TextDisabled("Used by the Anime renderer (Rendering menu), in the Game view.");

                ImGui::PushItemWidth(150.0f);

                ImGui::TextDisabled("Tones");

                ImGui::ColorEdit3("Base Color##Anime", animeSettings.base_color.data());
                tooltip("Tint of the lit tone. It multiplies the material's color and texture.");
                saveAfterEdit();

                ImGui::ColorEdit3("Shadow Color##Anime", animeSettings.shadow_color.data());
                tooltip("Tint of the shadow tone.");
                saveAfterEdit();

                ImGui::SliderFloat("Shadow Factor##Anime", &animeSettings.shadow_factor, 0.0f, 1.0f, "%.2f");
                tooltip("Darkens the shadow tone. 0 shows the shadow color as is, 1 turns the shadows black.");
                saveAfterEdit();

                ImGui::SliderFloat("Step##Anime", &animeSettings.step, 0.0f, 1.0f, "%.2f");
                tooltip("How much a surface has to face the light to be in the lit tone. Higher = more of it is in shadow.");
                saveAfterEdit();

                ImGui::SliderFloat("Light Falloff##Anime", &animeSettings.light_falloff, 0.0f, 1.0f, "%.2f");
                tooltip("0: two flat tones, like a cel.\n1: the lit tone fades into the shadow tone with the angle to the light, like diffuse shading.");
                saveAfterEdit();

                if (ImGui::Checkbox("Use Smooth Normal##Anime", &animeSettings.use_smooth_normal)) saveProjectSettings();
                tooltip("Cuts the tones with normals averaged over neighboring faces: a cleaner shadow edge on low-poly or hard-edged meshes.");

                ImGui::Spacing();
                ImGui::TextDisabled("Specular");

                if (ImGui::Checkbox("Enable Specular##Anime", &animeSettings.specular_enabled)) saveProjectSettings();
                tooltip("Highlights in the lit tone (Cook-Torrance). Their size follows the material's roughness and metallic,\nwhich the Model component can override per object.");

                ImGui::BeginDisabled(!animeSettings.specular_enabled);
                ImGui::SliderFloat("Strength##AnimeSpecular", &animeSettings.specular_strength, 0.0f, 4.0f, "%.2f");
                saveAfterEdit();
                ImGui::EndDisabled();

                ImGui::Spacing();
                ImGui::TextDisabled("Texture Blur");

                ImGui::SliderFloat("Base Color##AnimeBlur", &animeSettings.base_color_blur, 0.0f, 8.0f, "%.1f");
                tooltip("Smooths the base color texture by reading it from a lower mip level. 0 = sharp.");
                saveAfterEdit();

                ImGui::SliderFloat("Normal Map##AnimeBlur", &animeSettings.normal_map_blur, 0.0f, 8.0f, "%.1f");
                tooltip("The same for the normal map, which calms the highlights.");
                saveAfterEdit();

                ImGui::Spacing();
                ImGui::TextDisabled("Inner Edges");

                ImGui::ColorEdit3("Edge Color##Anime", animeSettings.inner_edge_color.data());
                saveAfterEdit();

                ImGui::SliderFloat("Inner Step##Anime", &animeSettings.inner_step, 0.0f, 1.0f, "%.2f");
                tooltip("Ink where a surface faces the camera less than this: a rim line inside the silhouette. 0 = off.");
                saveAfterEdit();

                ImGui::SliderFloat("Curve Step##Anime", &animeSettings.curve_step, 0.0f, 1.0f, "%.2f");
                tooltip("Ink where the mesh bends more than this (creases, folds). Lower = more lines. 1 = off.");
                saveAfterEdit();

                ImGui::Spacing();
                ImGui::TextDisabled("Outline");

                if (ImGui::Checkbox("Enable Outline##Anime", &animeSettings.outline_enabled)) saveProjectSettings();
                tooltip("Ink outline around each object's silhouette (inverted hull).");

                ImGui::BeginDisabled(!animeSettings.outline_enabled);
                ImGui::ColorEdit3("Color##AnimeOutline", animeSettings.outline_color.data());
                saveAfterEdit();

                ImGui::DragFloat("Thickness##AnimeOutline", &animeSettings.outline_thickness, 0.0005f, 0.0f, 1.0f, "%.4f");
                tooltip(animeSettings.outline_constant_screen_width
                    ? "Outline thickness as a fraction of the distance to the camera."
                    : "Outline thickness in world units.");
                saveAfterEdit();

                if (ImGui::Checkbox("Constant Screen Width##Anime", &animeSettings.outline_constant_screen_width)) saveProjectSettings();
                tooltip("Keeps the outline the same thickness on screen at any distance.");

                if (ImGui::Checkbox("Smooth Normals##AnimeOutline", &animeSettings.outline_smooth_normals)) saveProjectSettings();
                tooltip("Grows the outline along normals averaged over neighboring faces, which don't open gaps at hard edges.");
                ImGui::EndDisabled();

                ImGui::PopItemWidth();

                ImGui::Spacing();

                if (ImGui::Button("Reset Anime Shading to Defaults"))
                {
                    animeSettings = ProjectAnimeSettings{};
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