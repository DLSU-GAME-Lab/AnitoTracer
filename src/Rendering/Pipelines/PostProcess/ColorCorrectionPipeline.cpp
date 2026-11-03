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

    bool ColorCorrectionPipeline::Prepare(const ProjectSettings& projectSettings, const RenderData&, Uint32, Uint32)
    {
        const ProjectColorCorrectionSettings& settings = projectSettings.color_correction;
        if (!settings.enabled) return false;

        const float temperature = glm::clamp(settings.temperature, -1.0f, 1.0f);
        const float tint = glm::clamp(settings.tint, -1.0f, 1.0f);
        const glm::vec3 whiteBalance = NormalizeTint({ 1.0f + 0.3f * temperature, 1.0f - 0.3f * tint, 1.0f - 0.3f * temperature });
        const glm::vec3 filter(settings.color_filter[0], settings.color_filter[1], settings.color_filter[2]);

        ColorCorrectionConstants& cc = m_Constants;
        cc.ColorFilter = glm::vec4(whiteBalance * filter, std::exp2(settings.exposure));
        cc.Shadows = glm::vec4(NormalizeTint(settings.shadows), 1.0f);
        cc.Midtones = glm::vec4(NormalizeTint(settings.midtones), 1.0f);
        cc.Highlights = glm::vec4(NormalizeTint(settings.highlights), 1.0f);
        cc.Contrast = glm::max(settings.contrast, 0.0f);
        cc.Saturation = glm::max(settings.saturation, 0.0f);
        cc.InvGamma = 1.0f / glm::max(settings.gamma, 0.01f);
        cc.HueShift = glm::radians(glm::clamp(settings.hue_shift, -180.0f, 180.0f));
        return true;
    }

}
