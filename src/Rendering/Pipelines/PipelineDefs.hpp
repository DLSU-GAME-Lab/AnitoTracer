#pragma once
#include "BasePipeline.hpp"

namespace Diligent {

    static constexpr uint32_t MAX_DIR_LIGHTS = 4;
    static constexpr uint32_t MAX_POINT_LIGHTS = 8;

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

}
