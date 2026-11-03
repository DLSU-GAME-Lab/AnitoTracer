#pragma once

#include "Pipelines/Data/PipelineDefs.hpp"
#include "../Asset/ProjectSettings.hpp"

// Converts project settings into the GPU constant layouts shared by the scene passes (skybox and lit pipelines)
namespace Diligent {

    // Layout expected by Shaders/fog.hlsli. Wind.w carries the time since the first call, which animates the fog noise.
    FogConstants BuildFogConstants(const ProjectFogSettings& settings);

    ShadowSettings BuildShadowSettings(const ProjectLightingSettings& settings);

}
