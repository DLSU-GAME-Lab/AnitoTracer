#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED       

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    };

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

}
