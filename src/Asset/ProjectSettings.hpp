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

// HDR output. The scene is rendered in floating point, so lights and emissive surfaces can be brighter than white.
// The display pass at the end of the frame applies the exposure and a tone curve to fit that range onto the screen.
// Applies to the game view and release builds, and to the editor view when apply_to_editor_view is set.
// Stylized renderers (CrossHatch) are never tone mapped; their colors are already final.
struct ProjectHDRSettings
{
    int tone_mapper = 2;             // 0: off (clamp at white), 1: AgX, 2: Khronos PBR Neutral
    float exposure = 0.0f;           // In stops (EV), applied before the tone curve. 0 = unchanged.
    bool apply_to_editor_view = true;
    bool dither = true;              // Breaks up banding in smooth gradients on the 8-bit output

    // AgX look: a slope / offset / power / saturation grade applied inside the curve. These defaults are the plain "Base" look.
    std::array<float, 3> agx_slope{1.0f, 1.0f, 1.0f};
    float agx_offset = 0.0f;
    float agx_power = 1.0f;
    float agx_saturation = 1.0f;

    // Khronos PBR Neutral. These defaults are the values from the Khronos specification.
    float neutral_compression_start = 0.8f;  // Brightness where highlights start to roll off. Colors below it are left as authored.
    float neutral_desaturation = 0.15f;      // How quickly compressed highlights fade toward white
};

// Post-process color grading. Only applied to the game view / release build output, never the editor view.
// Exposure, white balance and the color filter act on the HDR scene, before the tone curve; the rest grades the tone mapped image.
struct ProjectColorCorrectionSettings
{
    bool enabled = false;
    float exposure = 0.0f;       // In stops (EV), on top of ProjectHDRSettings::exposure. 0 = unchanged.
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
    float threshold = 0.7f;      // Luminance below which pixels don't emit shafts. The scene is HDR, so this can be above 1.
    float threshold_knee = 0.3f; // Luminance range above the threshold over which pixels fade in to full emission.
    float max_brightness = 4.0f; // Caps how bright an emitting pixel counts, so a single very bright pixel can't streak across the screen.
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
    ProjectHDRSettings hdr;
    ProjectColorCorrectionSettings color_correction;
    ProjectGodRaysSettings god_rays;
    ProjectCrossHatchSettings crosshatch;
    ProjectDebugSettings debug;
    ProjectPhysicsSettings physics;
    ProjectAudioSettings audio;
};