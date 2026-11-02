#pragma once

#include <array>

struct ProjectLightingSettings
{
    float shadow_bias = 0.015f;
    float ambient_multiplier = 1.0f;
    std::array<float, 3> ambient_color{0.03f, 0.03f, 0.03f};
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

struct ProjectDebugSettings
{
    bool show_physics_collider_wireframes = false;
};

struct ProjectPhysicsSettings
{
    bool gravity_enabled = true;
    std::array<float, 3> gravity{0.0f, -9.81f, 0.0f};
};

struct ProjectSettings
{
    ProjectLightingSettings lighting;
    ProjectFogSettings fog;
    ProjectColorCorrectionSettings color_correction;
    ProjectDebugSettings debug;
    ProjectPhysicsSettings physics;
};