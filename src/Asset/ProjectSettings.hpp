#pragma once

#include <array>
#include <string>

struct ProjectLightingSettings
{
    float shadow_bias = 0.015f;
    float ambient_multiplier = 1.0f;
    std::array<float, 3> ambient_color{0.03f, 0.03f, 0.03f};

    // Hybrid renderer only: the sky (ProjectSkySettings) lights the scene and shows up in reflections.
    float sky_light_intensity = 1.0f;
    float reflection_intensity = 1.0f;  // Strength of all reflections; shown with ProjectReflectionSettings in the UI
};

// Ray-traced reflections of the Hybrid and Deferred Hybrid renderers.
// Every surface always reflects the sky. With these on, a ray is traced along each reflection to find what is
// really there: the sky, or an object, which then shows up lit, textured and shadowed.
struct ProjectReflectionSettings
{
    bool enabled = true;            // Trace reflection rays. Off: surfaces reflect the sky only, even with objects in the way.
    bool shade_hits = true;         // Show the objects rays hit. Off: an object only blocks the sky (the cheaper, older look).
    bool hit_shadows = true;        // Reflected objects keep their shadows, for one more ray per light at each hit.
    bool glossy = true;             // Rough surfaces blur what they reflect. Needs frames averaged: the denoiser, or TAA.
    float max_roughness = 0.6f;     // Surfaces rougher than this reflect the sky only. Rays fade out toward it.
    float max_distance = 100.0f;    // World units a ray travels looking for objects; beyond it the sky shows.
};

// Values of ProjectSkySettings::mode
enum ProjectSkyMode : int { SKY_MODE_TEXTURE = 0, SKY_MODE_SOLID_COLOR = 1 };

// File types the sky texture can be picked from, as a file dialog filter.
// .dds and .ktx hold cubemaps; the rest (and 2D .dds / .ktx) are read as 360 degree panoramas.
inline constexpr const char* kSkyTextureExtensions = "dds,ktx,hdr,png,jpg,jpeg,tga";

// The sky: what is drawn behind the scene and, in the ray-traced renderers, what lights it and shows in reflections.
struct ProjectSkySettings
{
    int mode = SKY_MODE_TEXTURE;

    // Texture mode: a cubemap or a 360 degree panorama. Empty uses the default skybox.
    // Stored relative to the project folder when the file is inside it, otherwise as picked.
    std::string texture_path;
    float rotation = 0.0f;                              // Degrees around the up axis

    // Solid color mode
    std::array<float, 3> color{0.35f, 0.55f, 0.9f};

    float intensity = 1.0f;                             // Brightness of the sky and of the light it casts
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
// Stylized renderers (CrossHatch, Anime) are never tone mapped; their colors are already final.
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

// Style of the Anime renderer (ported from Weeb_Big2). Colors are linear RGB, like the other settings.
struct ProjectAnimeSettings
{
    // Two tones: the lit tone where a surface faces the light by more than `step`, the shadow tone elsewhere
    std::array<float, 3> base_color{1.0f, 1.0f, 1.0f};          // Tints the lit tone (multiplies the material's color)
    std::array<float, 3> shadow_color{0.6f, 0.55f, 0.78f};      // Tints the shadow tone
    float shadow_factor = 0.25f;     // Darkens the shadow tone: 0 = the shadow color as is, 1 = black
    float step = 0.1f;               // N.L a surface needs to be in the lit tone
    float light_falloff = 0.0f;      // 0 = two flat tones, 1 = the lit tone fades into the shadow tone with N.L
    bool use_smooth_normal = false;  // Cut the tones with normals averaged over neighboring faces

    // Highlights (Cook-Torrance, sized by the material's roughness and metallic)
    bool specular_enabled = true;
    float specular_strength = 1.0f;

    // Texture blur, in mip levels: smooths texture detail, which reads as noise in a cel look. 0 = sharp.
    float base_color_blur = 0.0f;
    float normal_map_blur = 0.0f;

    // Inner edges: ink inside the silhouette
    std::array<float, 3> inner_edge_color{0.0f, 0.0f, 0.0f};
    float inner_step = 0.0f;         // Ink where a surface faces the camera less than this (0 = off)
    float curve_step = 0.5f;         // Ink where the mesh bends more than this (1 = off)

    // Inverted hull outline
    bool outline_enabled = true;
    std::array<float, 3> outline_color{0.0f, 0.0f, 0.0f};
    float outline_thickness = 0.02f;
    bool outline_constant_screen_width = false;  // Thickness scales with distance, so it is a fraction of the distance instead of world units
    bool outline_smooth_normals = true;          // Extrude along the smooth normals, which don't open gaps at hard edges
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

// Values of ProjectGISettings::method
enum ProjectGIMethod : int { GI_METHOD_SKY_AO = 0, GI_METHOD_PER_PIXEL = 1 };

// Indirect light in the ray-traced renderers: what surfaces receive from the sky and from each other,
// as opposed to straight from a light.
struct ProjectGISettings
{
    // 0, Sky + AO: flat ambient color plus sky light, dimmed by ray-traced ambient occlusion. No noise, no color bleeding.
    // 1, Per-pixel, one bounce: a ray per pixel gathers the light arriving from other surfaces and from the sky.
    //    Runs in Deferred Hybrid on devices where reflections can show objects; everywhere else Sky + AO is used.
    int method = GI_METHOD_PER_PIXEL;

    // Sky + AO in the Deferred Hybrid renderer: trace and filter the ambient occlusion for every 2x2 pixels instead of
    // every pixel, scaled back up afterwards. A quarter of the cost, for slightly softer shading in corners.
    bool ao_half_resolution = false;

    // The rest only applies to Per-pixel
    float intensity = 1.0f;         // Strength of the gathered light
    float max_distance = 100.0f;    // World units a GI ray travels looking for surfaces; beyond it the sky shows
    bool half_resolution = true;    // Trace for every 2x2 pixels instead of every pixel (a quarter of the rays), scaled back up afterwards
    int rays_per_pixel = 4;         // Rays per traced pixel every frame, 1 to 8. More is cleaner and steadier, and costs more.
    bool hit_shadows = true;        // Surfaces that bounce light keep their shadows, for one more ray per light
    bool more_bounces = true;       // Light keeps bouncing: surfaces that bounce light are also lit by last frame's result
    bool sky_bounce = true;         // Where that isn't known (off screen), they are lit by the sky if one more ray reaches it

    // How the denoiser treats the result (while ProjectDenoiseSettings has it on)
    float history_blend = 0.98f;    // Share of earlier frames kept every frame. Higher is smoother but slower to react.
    int filter_passes = 5;          // Blur passes over the result, 0 to 5. Each doubles the blur's reach.
    // The blur passes keep all the light they blur (SignalDenoiser::Settings::PreserveLight). Off is the blur as it was before,
    // which lost light where rays find a small bright area: a small glowing object lit its surroundings with half of its light.
    bool preserve_light = true;


};

// Cutout materials (fences, leaves) and blended ones (glass) in the ray-traced renderers.
// Off shows what they did before: every triangle blocks every ray, and blended surfaces are not drawn.
struct ProjectTransparencySettings
{
    bool enabled = true;            // Off: nothing below applies
    bool cutout_rays = true;        // Shadows, reflections and light bouncing around see the holes in cutout materials
    bool glass = true;              // Blended materials are drawn, as glass
    bool tinted_shadows = true;     // Light passes through glass, tinted by its color. Off: glass casts a solid shadow.
    bool refraction = true;         // Glass bends what is behind it. Off: plain alpha blending.
};

// How lights fall off, how big they are and what their brightness is measured in.
// The two switches that change every scene's brightness (inverse_square, real_units) are off by default, so a saved
// scene looks the same until a project asks for them. The rest only matters where a light has a size to show.
struct ProjectPhysicalLightSettings
{
    bool enabled = true;            // Off: lights behave as before this feature, whatever the switches below say
    bool inverse_square = false;    // Point and spot lights fall off as 1 / distance^2, smoothly cut to 0 at their range. Off: (1 - distance / range)^2.
    bool real_units = false;        // Brightness is set in lumens (point, spot and rectangular lights) and lux (directional lights), seen at ev100
    float ev100 = 15.0f;            // Exposure value (ISO 100) that real-unit lights are seen at: 15 is a sunny day, 6 to 8 a lit room
    bool light_size = true;         // Every light has a size of its own, and its shadows soften with it. Off: the old fixed sizes.
    bool highlight_shape = true;    // The size also widens the highlight a glossy surface shows
};

// What a material can be besides a plain metal or dielectric: a coat of clear lacquer, a layer of cloth fibres and a body
// that lets light through. Which of them a surface has comes from its material (glTF's KHR_materials_clearcoat, _sheen and
// _transmission) or from the object's overrides, so a scene without any shows only the specular anti-aliasing.
// Off shows each as before: no lobe, a transmissive material is an opaque one, and highlights are as sharp as the normals say.
struct ProjectMaterialSettings
{
    bool enabled = true;                // Off: nothing below applies
    bool specular_aa = true;            // Roughness grows where the normal changes quickly across a pixel, so highlights stop sparkling
    float specular_aa_strength = 1.0f;  // 1 filters as much as the normals' change says is needed; more is blurrier
    bool clearcoat = true;              // A second, smooth highlight over the base layer (car paint, lacquered wood)
    bool sheen = true;                  // A soft rim of light on cloth
    bool transmission = true;           // Materials that let the view through are drawn as glass (Hybrid and Deferred Hybrid)
};

// Denoising of the Deferred Hybrid renderer's ray-traced light. Every pixel traces one ray per light, one for its
// reflection and a few for indirect light. That is far too few on its own, so each result is averaged over frames
// and blurred along surfaces (our own filters, in the style of SVGF).
// Off shows the same rays as traced, noise included: for comparing against the filtered image.
struct ProjectDenoiseSettings
{
    bool enabled = true;        // Off: nothing is denoised, whatever the switches below say
    bool shadows = true;
    bool reflections = true;
    bool indirect = true;       // Per-pixel GI, or the ambient occlusion of Sky + AO
};

struct ProjectSettings
{
    ProjectLightingSettings lighting;
    ProjectReflectionSettings reflections;
    ProjectGISettings gi;
    ProjectDenoiseSettings denoise;
    ProjectTransparencySettings transparency;
    ProjectPhysicalLightSettings physical_lights;
    ProjectMaterialSettings materials;

    ProjectSkySettings sky;

    ProjectFogSettings fog;
    ProjectHDRSettings hdr;
    ProjectColorCorrectionSettings color_correction;
    ProjectGodRaysSettings god_rays;
    ProjectCrossHatchSettings crosshatch;
    ProjectAnimeSettings anime;
    ProjectDebugSettings debug;
    ProjectPhysicsSettings physics;
    ProjectAudioSettings audio;
};