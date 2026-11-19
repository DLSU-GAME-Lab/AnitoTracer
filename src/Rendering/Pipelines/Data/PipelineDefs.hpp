#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED       

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../../Common/DebugLine.hpp"

namespace Diligent {

    static constexpr uint32_t MAX_DIR_LIGHTS = 4;
    static constexpr uint32_t MAX_POINT_LIGHTS = 8;

    // Saved in UserSettings, so new values go at the end
    enum PipelineType { BASIC_LIT, HYBRID, DEFERRED, DEFERRED_HYBRID, CROSSHATCH, CROSSHATCH_RT };

    struct DirectionalLightData {
        glm::vec4 Direction{ 0.0f, -1.0f, 0.0f, 0.0f }; // xyz: dir, w: unused
        glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };      // rgb: color, a: intensity
    };

    struct PointLightData {
        glm::vec4 Position{ 0.0f, 0.0f, 0.0f, 1.0f };   // xyz: pos, w: unused
        glm::vec4 Color{ 1.0f, 1.0f, 1.0f, 1.0f };      // rgb: color, a: intensity
        float Range = 10.0f;
        glm::vec3 Padding{ 0.0f };
        glm::vec4 ExtraPadding{ 0.0f };  // Additional padding for HLSL std140 alignment (8 lights × 16 bytes = 128 byte difference)
    };

    struct LightConstants {
        DirectionalLightData DirLights[MAX_DIR_LIGHTS];
        PointLightData PointLights[MAX_POINT_LIGHTS];
        int NumDirLights = 0;
        int NumPointLights = 0;
        glm::vec2 Padding{ 0.0f };
        glm::vec4 CameraPos{ 0.0f };
    };

    struct ShadowSettings
    {
        float ShadowBias = 0.015f;
        float AmbientMultiplier = 1.0f;
        float SkyLightIntensity = 1.0f;   // Hybrid only
        float ReflectionIntensity = 1.0f; // Hybrid only
        glm::vec4 AmbientColor{ 0.03f, 0.03f, 0.03f, 1.0f };
        uint32_t NoiseFrame = 0;          // RenderData::NoiseFrame: varies ray sample patterns per frame under TAA, 0 keeps them still
        glm::vec2 SkyRotation{ 0.0f, 1.0f }; // Hybrid only: (sin, cos) of the sky's rotation around the up axis
        float ReflectionMaxRoughness = 0.6f; // Hybrid only: surfaces rougher than this reflect the sky without tracing a ray

        float ReflectionMaxDistance = 100.0f; // Hybrid only: how far a reflection ray looks for geometry
        uint32_t ReflectionFlags = 0;         // Hybrid only: REFLECTION_* bits below
        glm::vec2 Padding{ 0.0f };
    };
    static_assert(sizeof(ShadowSettings) % 16 == 0, "ShadowSettings must be 16-byte aligned for HLSL");

    // Bits of ShadowSettings::ReflectionFlags. Mirror the REFLECTION_* constants in Shaders/hybrid_shading.hlsli
    enum ReflectionFlags : uint32_t {
        REFLECTION_TRACE = 1u << 0,        // Trace reflection rays; without it surfaces reflect only the sky
        REFLECTION_SHADE_HITS = 1u << 1,   // Light and texture what a ray hits; without it a hit only blocks the sky
        REFLECTION_HIT_SHADOWS = 1u << 2,  // Trace shadow rays from what a ray hits
        REFLECTION_GLOSSY = 1u << 3,       // Spread rays over rough surfaces' highlight (only while frames are averaged: by TAA or a denoiser)
    };

    // What a ray hit needs to know about the triangle it landed on: where its vertices are in SceneGeometryPool and
    // the material to shade it with. SceneTLAS writes one per (model instance, submesh) every frame.
    // Mirrors SceneGeometryRecord in Shaders/rt_scene.hlsli.
    struct SceneGeometryRecord
    {
        uint32_t FirstIndex = 0;                // Of the submesh, in the pool's index buffer
        uint32_t BaseVertex = 0;                // Of the model, in the pool's vertex buffer; added to every index
        int32_t BaseColorTexture = -1;          // Slots in the scene texture array, -1: no map
        int32_t MetallicRoughnessTexture = -1;

        glm::vec4 BaseColor{ 1.0f };

        glm::vec3 Emissive{ 0.0f };             // Color * strength
        int32_t EmissiveTexture = -1;

        float Roughness = 1.0f;
        float Metallic = 0.0f;
        uint32_t Flags = 0;                     // SCENE_GEOMETRY_* bits below
        float Padding = 0.0f;
    };
    static_assert(sizeof(SceneGeometryRecord) == 64, "SceneGeometryRecord must match Shaders/rt_scene.hlsli");

    // Bits of SceneGeometryRecord::Flags. Mirror the SCENE_GEOMETRY_* constants in Shaders/rt_scene.hlsli
    enum SceneGeometryFlags : uint32_t {
        SCENE_GEOMETRY_IN_POOL = 1u << 0,           // FirstIndex / BaseVertex are valid; otherwise the hit can't be shaded
        SCENE_GEOMETRY_ROUGHNESS_FIXED = 1u << 1,   // Roughness is a per-object override: ignore the texture's channel
        SCENE_GEOMETRY_METALLIC_FIXED = 1u << 2,    // Same for metallic
    };

    // Mirrors cbuffer SkyboxConstants in Shaders/Skybox/Skybox_ps.hlsl
    struct SkyboxConstants
    {
        glm::vec2 Rotation{ 0.0f, 1.0f };   // (sin, cos) of the sky's rotation around the up axis
        float Intensity = 1.0f;             // Brightness multiplier
        float Padding = 0.0f;
    };
    static_assert(sizeof(SkyboxConstants) % 16 == 0, "SkyboxConstants must be 16-byte aligned for HLSL");

    // Mirrors cbuffer FogConstants in Shaders/fog.hlsli
    struct FogConstants
    {
        glm::vec4 LinearColor{ 0.6f, 0.65f, 0.7f, 1.0f };     // rgb: color, a: max opacity
        glm::vec4 VolumetricColor{ 0.8f, 0.85f, 0.9f, 0.0f }; // rgb: scattering tint, a: density
        float LinearStart = 10.0f;
        float LinearEnd = 100.0f;
        float HeightFalloff = 0.15f;
        float BaseHeight = 0.0f;
        float Anisotropy = 0.4f;
        float MaxDistance = 150.0f;
        float NoiseStrength = 0.5f;
        float NoiseScale = 0.08f;
        glm::vec4 Wind{ 0.0f };                                 // xyz: wind velocity, w: time in seconds
        int LinearEnabled = 0;
        int VolumetricEnabled = 0;
        int ApplyToSkybox = 1;
        int SampleCount = 24;
        float LightIntensity = 1.0f;
        float AmbientIntensity = 0.3f;
        uint32_t NoiseFrame = 0;                                // RenderData::NoiseFrame: varies the march jitter per frame under TAA
        float Padding = 0.0f;
    };
    static_assert(sizeof(FogConstants) % 16 == 0, "FogConstants must be 16-byte aligned for HLSL");

    // Tone curves of the display pass. Values match ProjectHDRSettings::tone_mapper and Shaders/PostProcess/tonemap.hlsli
    enum ToneMapper : int32_t { TONE_MAPPER_OFF = 0, TONE_MAPPER_AGX = 1, TONE_MAPPER_PBR_NEUTRAL = 2, TONE_MAPPER_COUNT };

    // Mirrors cbuffer ColorCorrectionConstants in Shaders/PostProcess/color_correction_ps.hlsl
    struct ColorCorrectionConstants
    {
        glm::vec4 ColorFilter{ 1.0f };      // rgb: multiplier (white balance already folded in), a: exposure multiplier
        glm::vec4 Shadows{ 1.0f };          // rgb: lift tint
        glm::vec4 Midtones{ 1.0f };         // rgb: gamma tint
        glm::vec4 Highlights{ 1.0f };       // rgb: gain tint
        float Contrast = 1.0f;
        float Saturation = 1.0f;
        float InvGamma = 1.0f;
        float HueShift = 0.0f;              // Radians

        glm::vec4 AgXSlope{ 1.0f, 1.0f, 1.0f, 0.0f };   // rgb: slope, a: offset
        float AgXPower = 1.0f;
        float AgXSaturation = 1.0f;
        float NeutralCompressionStart = 0.8f;
        float NeutralDesaturation = 0.15f;

        int32_t ToneMapper = TONE_MAPPER_OFF;
        int32_t GradingEnabled = 0;         // 0 skips everything graded after the tone curve
        float DitherStrength = 0.0f;        // In 8-bit output steps; 0 = off
        float Padding = 0.0f;
    };
    static_assert(sizeof(ColorCorrectionConstants) % 16 == 0, "ColorCorrectionConstants must be 16-byte aligned for HLSL");

    // Mirrors cbuffer GodRaysConstants in Shaders/PostProcess/god_rays_ps.hlsl
    struct GodRaysConstants
    {
        glm::vec4 LightColor{ 0.0f };       // rgb: light color * intensity, a: on-screen fade
        glm::vec2 SunUV{ 0.5f };            // Sun position in UV space; may lie outside 0..1
        float Density = 0.8f;
        float Decay = 0.96f;                // Per-sample, already adjusted for SampleCount
        float Weight = 0.03f;               // Per-sample, already adjusted for SampleCount
        float Threshold = 0.7f;
        float SunRadius = 0.35f;
        float AspectRatio = 1.0f;
        int SampleCount = 64;
        float Knee = 0.3f;                  // Luminance range above Threshold over which pixels fade in
        float MaxBrightness = 4.0f;         // Luminance cap of an emitting pixel
        float Padding = 0.0f;
    };
    static_assert(sizeof(GodRaysConstants) % 16 == 0, "GodRaysConstants must be 16-byte aligned for HLSL");

    // Mirrors cbuffer TemporalAAConstants in Shaders/PostProcess/taa_ps.hlsl. Matrices are transposed like CameraConstants.
    struct TemporalAAConstants
    {
        glm::mat4 InvViewProj{ 1.0f };      // This frame's jittered clip space -> world
        glm::mat4 ViewProj{ 1.0f };         // This frame's unjittered world -> clip space
        glm::mat4 PrevViewProj{ 1.0f };     // Last frame's unjittered world -> clip space
        glm::vec4 ViewportSize{ 0.0f };     // xy: size in pixels, zw: 1 / size
        float HistoryBlend = 0.9f;
        float Sharpness = 0.0f;
        float HistoryValid = 0.0f;          // 0 on the first frame and after a reset: output this frame as is
        float HasDepth = 0.0f;              // 0 reprojects every pixel as if it were at the far plane
        float HasVelocity = 0.0f;           // 1 when the pipeline supplied motion vectors, which also follow moving objects
        glm::vec3 Padding{ 0.0f };
    };
    static_assert(sizeof(TemporalAAConstants) % 16 == 0, "TemporalAAConstants must be 16-byte aligned for HLSL");
    // Mirrors cbuffer CrossHatchConstants in Shaders/CrossHatch/crosshatch_common.hlsli
    struct CrossHatchConstants
    {
        glm::vec4 InkColor{ 0.0f, 0.0f, 0.0f, 1.0f };   // rgb: ink color, a: hatch opacity
        glm::vec4 CameraPos{ 0.0f };

        float LitScale = 0.4f;
        float LitThickness = 0.3f;
        float LitDensity = 1.0f;
        float LitAngle = 0.785f;

        float DetailScale = 0.5f;
        float DetailDensity = 0.25f;
        float DetailAngle = 2.983f;
        float DetailThickness = 10.0f;

        float ShadowScale = 0.4f;
        float ShadowDensity = 2.0f;
        float ShadowAngle = 2.35f;
        float ShadowThickness = 0.3f;

        float LineSmoothness = 0.02f;
        float ShadowSmoothness = 0.02f;
        float ShadowCoreBias = 0.3f;
        float ShadowIntensity = 1.0f;

        float TexScale = 6.0f;
        float TexPriority = 0.5f;
        float TexBlackThreshold = 0.2f;
        float TexSmoothness = 0.05f;

        float TexRotation = 0.0f;
        float TexThickness = 0.0f;
        float TexLightBias = 0.2f;
        float SpecularThreshold = 0.85f;

        float OutlineWidth = 0.003f;
        float OutlineMode = 0.0f;               // 0: along normals, 1: away from the object's origin
        float OutlineScaleWithDistance = 1.0f;
        float Padding0 = 0.0f;

        int HatchMode = 0;                      // 0: noise, 1: toon texture, 2: toon + noise
        int InnerEdges = 1;
        int SpecularEnabled = 0;
        int Padding1 = 0;
    };
    static_assert(sizeof(CrossHatchConstants) % 16 == 0, "CrossHatchConstants must be 16-byte aligned for HLSL");

    struct alignas(16) PBRMaterialConstants {
        glm::vec4 BaseColorFactor{ 1.0f, 1.0f, 1.0f, 1.0f };
        float MetallicFactor{ 1.0f };
        float RoughnessFactor{ 1.0f };
        float UseBaseColorMap{ 0.0f };
        float UseMetallicRoughnessMap{ 0.0f };

        float UseNormalMap{ 0.0f };
        float UseAOMap{ 0.0f };
        float UseEmissiveMap{ 0.0f };
        float OverrideFlags{ 0.0f }; // Bit 1: roughness overridden, bit 2: metallic overridden (skip the texture channel)

        glm::vec4 EmissiveFactor{ 0.0f }; // rgb: emissive color * strength (HDR), multiplied with the emissive map
    };

    // Mirrors cbuffer CameraConstants in Shaders/common_struct.hlsli. Shaders that only declare View and Proj read the start of it.
    struct CameraConstants {
        glm::mat4 View;
        glm::mat4 Proj;
        // For motion vectors: world -> clip space this frame and last frame, both without TAA's sub-pixel jitter
        glm::mat4 ViewProjNoJitter{ 1.0f };
        glm::mat4 PrevViewProjNoJitter{ 1.0f };
    };

    // Mirrors cbuffer ModelConstants in Shaders/common_struct.hlsli
    struct ModelConstants {
        glm::mat4 Model;
        glm::mat4 NormalMatrix;
        glm::mat4 PrevModel{ 1.0f };    // Last frame's Model, for motion vectors
    };

}
