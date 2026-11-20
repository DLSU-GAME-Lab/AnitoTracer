#pragma once

#include "Pipelines/Data/PipelineDefs.hpp"
#include "../Asset/ProjectSettings.hpp"

// Converts project settings into the GPU constant layouts shared by the scene passes (skybox and lit pipelines)
namespace Diligent {

    // Layout expected by Shaders/fog.hlsli. Wind.w carries the time since the first call, which animates the fog noise.
    // noiseFrame is RenderData::NoiseFrame, which moves the volumetric march jitter every frame under TAA.
    FogConstants BuildFogConstants(const ProjectFogSettings& settings, uint32_t noiseFrame = 0);

    // noiseFrame is RenderData::NoiseFrame, which moves the ray-traced shadow and AO sample patterns every frame under TAA.
    // The sky's intensity and rotation are folded in, so sky lighting follows the skybox.
    ShadowSettings BuildShadowSettings(const ProjectLightingSettings& lighting, const ProjectSkySettings& sky,
        const ProjectReflectionSettings& reflections, const ProjectTransparencySettings& transparency, uint32_t noiseFrame = 0);


    // Layout expected by Shaders/Skybox/Skybox_ps.hlsl
    SkyboxConstants BuildSkyboxConstants(const ProjectSkySettings& settings);

    // Layout expected by Shaders/CrossHatch/crosshatch_common.hlsli. CameraPos is left for the pipeline to fill.
    CrossHatchConstants BuildCrossHatchConstants(const ProjectCrossHatchSettings& settings);

}
