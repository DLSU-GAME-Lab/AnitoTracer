#include "ColorCorrectionPipeline.hpp"
#include "../../../Asset/ProjectSettings.hpp"
#include <cmath>

namespace {
    // Scales a tint so it shifts hue without changing brightness; black/invalid tints fall back to neutral
    glm::vec3 NormalizeTint(const std::array<float, 3>& tint)
    {
        const glm::vec3 color(tint[0], tint[1], tint[2]);
        const float luminance = glm::dot(color, glm::vec3(0.2126f, 0.7152f, 0.0722f));
        return luminance > 1e-3f ? color / luminance : glm::vec3(1.0f);
    }
}

namespace Diligent {

    // Point sampling: the intermediate matches the destination 1:1
    ColorCorrectionPipeline::ColorCorrectionPipeline()
        : PostProcessEffect("Color Correction", "PostProcess/color_correction_ps.hlsl", "ColorCorrectionConstants", PipelineUtils::PointClampSampler()) {}

    bool ColorCorrectionPipeline::Prepare(const ProjectSettings& projectSettings, const RenderData&, const PostProcessView& view, Uint32, Uint32)
    {
        const ProjectHDRSettings& hdr = projectSettings.hdr;
        const ProjectColorCorrectionSettings& grading = projectSettings.color_correction;
        const bool gradingEnabled = view.Effects && grading.enabled;

        ColorCorrectionConstants cc; // Neutral defaults: no grading, no tone curve

        // Scene exposure and tone curve
        float exposureEV = 0.0f;
        if (view.ToneMap) {
            exposureEV += glm::clamp(hdr.exposure, -20.0f, 20.0f);
            cc.ToneMapper = glm::clamp(hdr.tone_mapper, 0, static_cast<int>(TONE_MAPPER_COUNT) - 1);
        }
        cc.AgXSlope = glm::vec4(glm::max(glm::vec3(hdr.agx_slope[0], hdr.agx_slope[1], hdr.agx_slope[2]), 0.0f), hdr.agx_offset);
        cc.AgXPower = glm::max(hdr.agx_power, 0.01f);
        cc.AgXSaturation = glm::max(hdr.agx_saturation, 0.0f);
        // The curve needs some room between where compression starts and white (1.0) to roll off into
        cc.NeutralCompressionStart = glm::clamp(hdr.neutral_compression_start, 0.1f, 0.99f);
        cc.NeutralDesaturation = glm::max(hdr.neutral_desaturation, 0.0f);
        cc.DitherStrength = hdr.dither ? 1.0f : 0.0f;

        // Color grading
        cc.GradingEnabled = gradingEnabled ? 1 : 0;
        glm::vec3 sceneFilter(1.0f);
        if (gradingEnabled) {
            const float temperature = glm::clamp(grading.temperature, -1.0f, 1.0f);
            const float tint = glm::clamp(grading.tint, -1.0f, 1.0f);
            const glm::vec3 whiteBalance = NormalizeTint({ 1.0f + 0.3f * temperature, 1.0f - 0.3f * tint, 1.0f - 0.3f * temperature });
            sceneFilter = whiteBalance * glm::vec3(grading.color_filter[0], grading.color_filter[1], grading.color_filter[2]);
            exposureEV += grading.exposure;

            cc.Shadows = glm::vec4(NormalizeTint(grading.shadows), 1.0f);
            cc.Midtones = glm::vec4(NormalizeTint(grading.midtones), 1.0f);
            cc.Highlights = glm::vec4(NormalizeTint(grading.highlights), 1.0f);
            cc.Contrast = glm::max(grading.contrast, 0.0f);
            cc.Saturation = glm::max(grading.saturation, 0.0f);
            cc.InvGamma = 1.0f / glm::max(grading.gamma, 0.01f);
            cc.HueShift = glm::radians(glm::clamp(grading.hue_shift, -180.0f, 180.0f));
        }
        cc.ColorFilter = glm::vec4(sceneFilter, std::exp2(exposureEV));

        m_Constants = cc;
        return true;
    }

}
