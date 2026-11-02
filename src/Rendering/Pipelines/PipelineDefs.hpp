#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED       

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../../Common/DebugLine.hpp"

namespace Diligent {

    static constexpr uint32_t MAX_DIR_LIGHTS = 4;
    static constexpr uint32_t MAX_POINT_LIGHTS = 8;

    enum PipelineType { BASIC_LIT, HYBRID, DEFERRED };

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
        float Padding[2] = { 0.0f, 0.0f }; // Pads the struct size to 16 bytes
        glm::vec4 AmbientColor{ 0.03f, 0.03f, 0.03f, 1.0f };
    };

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
        float Padding[2] = { 0.0f, 0.0f };
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

    struct alignas(16) PBRMaterialConstants {
        glm::vec4 BaseColorFactor{ 1.0f, 1.0f, 1.0f, 1.0f };
        float MetallicFactor{ 1.0f };
        float RoughnessFactor{ 1.0f };
        float UseBaseColorMap{ 0.0f };
        float UseMetallicRoughnessMap{ 0.0f };

        float UseNormalMap{ 0.0f };
        float UseAOMap{ 0.0f };
        float UseEmissiveMap{ 0.0f };
        float PaddingMat{ 0.0f };
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
