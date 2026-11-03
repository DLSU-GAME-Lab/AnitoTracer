#pragma once

#include "PostProcessPipeline.hpp"
#include "../Data/PipelineDefs.hpp"

namespace Diligent {

    // Screen-space light shafts from the main directional light, added on top of the scene (Shaders/PostProcess/god_rays_ps.hlsl)
    class GodRaysPipeline : public PostProcessEffect<GodRaysConstants> {
    public:
        GodRaysPipeline();

        // Skipped when disabled, with no directional light, or when the sun is behind the camera
        bool Prepare(const ProjectSettings& settings, const RenderData& renderData, Uint32 width, Uint32 height) override;
    };

}
