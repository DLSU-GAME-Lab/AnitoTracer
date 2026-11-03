#pragma once

#include "PostProcessPipeline.hpp"
#include "../Data/PipelineDefs.hpp"

namespace Diligent {

    // Color grading of the game output (Shaders/PostProcess/color_correction_ps.hlsl)
    class ColorCorrectionPipeline : public PostProcessEffect<ColorCorrectionConstants> {
    public:
        ColorCorrectionPipeline();

        bool Prepare(const ProjectSettings& settings, const RenderData& renderData, Uint32 width, Uint32 height) override;
    };

}
