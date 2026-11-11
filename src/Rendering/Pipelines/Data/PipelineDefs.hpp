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
        float Padding[3] = { 0.0f, 0.0f, 0.0f };
    };
    static_assert(sizeof(ShadowSettings) % 16 == 0, "ShadowSettings must be 16-byte aligned for HLSL");

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
        float Padding[3] = { 0.0f, 0.0f, 0.0f };
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

    struct CameraConstants {
        glm::mat4 View;
        glm::mat4 Proj;
    };

    struct ModelConstants {
        glm::mat4 Model;
        glm::mat4 NormalMatrix;
    };

}
