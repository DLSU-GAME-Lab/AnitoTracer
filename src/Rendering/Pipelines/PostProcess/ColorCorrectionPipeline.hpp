#pragma once

#include "PostProcessPipeline.hpp"
#include "../Data/PipelineDefs.hpp"

namespace Diligent {

    // Display pass, always the last stage of the PostProcessStack (Shaders/PostProcess/color_correction_ps.hlsl).
    // Converts the HDR scene into the 8-bit output: exposure -> tone curve (ProjectHDRSettings) -> dither.
    // Views with effects also get the project's color grading (ProjectColorCorrectionSettings) around the tone curve.
    class ColorCorrectionPipeline : public PostProcessEffect<ColorCorrectionConstants> {
    public:
        ColorCorrectionPipeline();

        // Never skipped: without it nothing would write the output
        bool Prepare(const ProjectSettings& settings, const RenderData& renderData, const PostProcessView& view, Uint32 width, Uint32 height) override;
    };

}
