#pragma once

#include <array>

struct ProjectLightingSettings
{
    float shadow_bias = 0.015f;
    float ambient_multiplier = 1.0f;
    std::array<float, 3> ambient_color{0.03f, 0.03f, 0.03f};

    // Hybrid renderer only: the skybox lights the scene and shows up in reflections.
    float sky_light_intensity = 1.0f;
    float reflection_intensity = 1.0f;
};

struct ProjectFogSettings
{
    // Linear (distance) fog: blends toward a flat color between start and end distance.
    bool linear_enabled = false;
    std::array<float, 3> linear_color{0.6f, 0.65f, 0.7f};
    float linear_start = 10.0f;
    float linear_end = 100.0f;
    float linear_max_opacity = 1.0f;

    // Volumetric fog: ray-marched height fog that scatters scene lights.
    bool volumetric_enabled = false;
    std::array<float, 3> volumetric_color{0.8f, 0.85f, 0.9f};
    float volumetric_density = 0.03f;
    float volumetric_height_falloff = 0.15f;
    float volumetric_base_height = 0.0f;
    float volumetric_anisotropy = 0.4f;
    float volumetric_max_distance = 150.0f;
    int volumetric_sample_count = 24;
    float volumetric_light_intensity = 1.0f;
    float volumetric_ambient_intensity = 0.3f;
    float volumetric_noise_strength = 0.5f;
    float volumetric_noise_scale = 0.08f;
    std::array<float, 3> volumetric_wind{0.5f, 0.0f, 0.25f};

    bool apply_to_skybox = true;
};

// Post-process color grading. Only applied to the game view / release build output, never the editor view.
struct ProjectColorCorrectionSettings
{
    bool enabled = false;
    float exposure = 0.0f;       // In stops (EV). 0 = unchanged.
    float contrast = 1.0f;       // 1 = unchanged. Pivots around mid-grey.
    float saturation = 1.0f;     // 0 = greyscale, 1 = unchanged.
    float gamma = 1.0f;          // 1 = unchanged. >1 brightens midtones.
    float temperature = 0.0f;    // -1 (cool) .. 1 (warm)
    float tint = 0.0f;           // -1 (green) .. 1 (magenta)
    float hue_shift = 0.0f;      // Degrees, -180 .. 180
    std::array<float, 3> color_filter{1.0f, 1.0f, 1.0f};

    // Lift / gamma / gain style three-way grading for shadows, midtones and highlights.
    std::array<float, 3> shadows{1.0f, 1.0f, 1.0f};
    std::array<float, 3> midtones{1.0f, 1.0f, 1.0f};
    std::array<float, 3> highlights{1.0f, 1.0f, 1.0f};
};

// Screen-space light shafts from the first directional light. Only applied to the game view / release build output.
struct ProjectGodRaysSettings
{
    bool enabled = false;
    float intensity = 1.0f;
    float density = 0.8f;        // Fraction of the way to the sun each pixel marches. Longer shafts at higher values.
    float decay = 0.96f;         // Falloff along the shaft (per 64 samples). Closer to 1 = longer shafts.
    float threshold = 0.7f;      // Luminance below which pixels don't emit shafts.
    float sun_radius = 0.35f;    // Size of the emitting region around the sun, as a fraction of screen height.
    int sample_count = 64;
    bool use_light_color = true; // Multiply the tint by the directional light's color and intensity.
    std::array<float, 3> tint{1.0f, 0.95f, 0.85f};
};

// Style of the CrossHatch renderers (ported from AnitoCrossHatch). Angles are in radians.
struct ProjectCrossHatchSettings
{
    int mode = 1;                    // 0: noise hatching, 1: toon texture hatching, 2: toon + noise hatching
    std::array<float, 3> ink_color{0.0f, 0.0f, 0.0f};
    float hatch_opacity = 1.0f;      // 0 = plain lit color, 1 = full hatching
    bool inner_edges = true;         // Ink along creases where the normal bends sharply
    int noise_texture = 1;           // Assets/CrossHatch/Noise/noise<N>.dds, 1..11. Shapes the noise strokes.

    // Noise hatching, lit layer: strokes that grow denser as the surface darkens
    float lit_scale = 0.4f;
    float lit_thickness = 0.3f;
    float lit_density = 1.0f;
    float lit_angle = 0.785f;
    float line_smoothness = 0.02f;

    // Noise hatching, detail layer: a constant layer of strokes on top
    float detail_scale = 0.5f;
    float detail_density = 0.25f;
    float detail_angle = 2.983f;
    float detail_thickness = 10.0f;

    // Noise hatching, shadow layer: strokes inside the key light's shadow (cast shadows need the ray-traced renderer)
    float shadow_scale = 0.4f;
    float shadow_density = 2.0f;
    float shadow_angle = 2.35f;
    float shadow_thickness = 0.3f;
    float shadow_smoothness = 0.02f;
    float shadow_core_bias = 0.3f;
    float shadow_intensity = 1.0f;

    // Toon texture hatching
    float texture_scale = 6.0f;
    float texture_priority = 0.5f;   // Weight of the cross strokes. Lower = they appear only in darker areas.
    float texture_black_threshold = 0.2f;
    float texture_smoothness = 0.05f;
    float texture_rotation = 0.0f;
    float texture_thickness = 0.0f;
    float texture_light_bias = 0.2f;
    bool specular_enabled = false;
    float specular_threshold = 0.85f;

    // Inverted hull outline
    bool outline_enabled = true;
    float outline_width = 0.003f;
    bool outline_constant_screen_width = true; // Width scales with distance, so it is a fraction of the distance instead of world units
    bool outline_from_origin = false;          // Extrude away from the object's origin instead of along normals (hard-surface meshes)
};

struct ProjectDebugSettings
{
    bool show_physics_collider_wireframes = false;
};

struct ProjectPhysicsSettings
{
    bool gravity_enabled = true;
    std::array<float, 3> gravity{0.0f, -9.81f, 0.0f};
};

struct ProjectAudioSettings
{
    float master_volume = 100.0f;
    float bgm_volume = 100.0f;
    float sfx_volume = 100.0f;
};

struct ProjectSettings
{
    ProjectLightingSettings lighting;
    ProjectFogSettings fog;
    ProjectColorCorrectionSettings color_correction;
    ProjectGodRaysSettings god_rays;
    ProjectCrossHatchSettings crosshatch;
    ProjectDebugSettings debug;
    ProjectPhysicsSettings physics;
    ProjectAudioSettings audio;
};