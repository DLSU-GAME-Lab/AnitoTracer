#include "GodRaysPipeline.hpp"
#include "../../RenderData.hpp"
#include "../../../Asset/ProjectSettings.hpp"
#include <cmath>

namespace Diligent {

    // Linear filtering smooths the radial march between texels
    GodRaysPipeline::GodRaysPipeline()
        : PostProcessEffect("God Rays", "PostProcess/god_rays_ps.hlsl", "GodRaysConstants", PipelineUtils::LinearClampSampler()) {}

    bool GodRaysPipeline::Prepare(const ProjectSettings& projectSettings, const RenderData& renderData, Uint32 width, Uint32 height)
    {
        const ProjectGodRaysSettings& settings = projectSettings.god_rays;
        if (!settings.enabled) return false;
        if (renderData.Lights.NumDirLights <= 0 || width == 0 || height == 0) return false;

        const auto& sun = renderData.Lights.DirLights[0];
        const glm::vec3 toSun = -glm::normalize(glm::vec3(sun.Direction));

        // w = 0 projects the direction itself, i.e. a point infinitely far away
        const glm::vec4 viewDir = renderData.ViewMatrix * glm::vec4(toSun, 0.0f);
        const glm::vec4 clip = renderData.ProjectionMatrix * viewDir;
        if (clip.w <= 1e-4f) return false;

        GodRaysConstants& gr = m_Constants;
        const glm::vec2 ndc = glm::vec2(clip) / clip.w;
        gr.SunUV = glm::vec2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);

        // Fade out as the sun approaches the camera's side plane or drifts far off-screen
        const float facingFade = glm::smoothstep(0.0f, 0.25f, glm::normalize(glm::vec3(viewDir)).z);
        const float edgeFade = 1.0f - glm::smoothstep(1.0f, 2.0f, glm::max(glm::abs(ndc.x), glm::abs(ndc.y)));

        glm::vec3 color(settings.tint[0], settings.tint[1], settings.tint[2]);
        if (settings.use_light_color) color *= glm::vec3(sun.Color) * sun.Color.a;

        const int sampleCount = glm::clamp(settings.sample_count, 8, 128);
        const float decay = std::pow(glm::clamp(settings.decay, 0.5f, 0.999f), 64.0f / sampleCount);

        gr.LightColor = glm::vec4(color, glm::max(settings.intensity, 0.0f) * facingFade * edgeFade);
        gr.Density = glm::clamp(settings.density, 0.0f, 1.0f);
        gr.Decay = decay;
        // Normalizes the decaying sum of all samples to 1, so decay and sample count only change the shape, not the brightness
        gr.Weight = (1.0f - decay) / (1.0f - std::pow(decay, static_cast<float>(sampleCount)));
        gr.Threshold = glm::clamp(settings.threshold, 0.0f, 0.99f);
        gr.SunRadius = glm::max(settings.sun_radius, 0.01f);
        gr.AspectRatio = static_cast<float>(width) / static_cast<float>(height);
        gr.SampleCount = sampleCount;

        // Nothing visible to add (sun at the edge of view or zero intensity)
        return gr.LightColor.a > 0.0f;
    }

}
