#include "SceneConstants.hpp"
#include <chrono>

namespace Diligent {

    FogConstants BuildFogConstants(const ProjectFogSettings& settings, uint32_t noiseFrame)
    {
        static const auto startTime = std::chrono::steady_clock::now();
        const float elapsedSeconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();

        FogConstants fog;
        fog.LinearColor = glm::vec4(settings.linear_color[0], settings.linear_color[1], settings.linear_color[2], settings.linear_max_opacity);
        fog.VolumetricColor = glm::vec4(settings.volumetric_color[0], settings.volumetric_color[1], settings.volumetric_color[2], settings.volumetric_density);
        fog.LinearStart = settings.linear_start;
        fog.LinearEnd = settings.linear_end;
        fog.HeightFalloff = settings.volumetric_height_falloff;
        fog.BaseHeight = settings.volumetric_base_height;
        fog.Anisotropy = glm::clamp(settings.volumetric_anisotropy, -0.95f, 0.95f);
        fog.MaxDistance = settings.volumetric_max_distance;
        fog.NoiseStrength = settings.volumetric_noise_strength;
        fog.NoiseScale = settings.volumetric_noise_scale;
        fog.Wind = glm::vec4(settings.volumetric_wind[0], settings.volumetric_wind[1], settings.volumetric_wind[2], elapsedSeconds);
        fog.LinearEnabled = settings.linear_enabled ? 1 : 0;
        fog.VolumetricEnabled = settings.volumetric_enabled ? 1 : 0;
        fog.ApplyToSkybox = settings.apply_to_skybox ? 1 : 0;
        fog.SampleCount = glm::clamp(settings.volumetric_sample_count, 1, 128);
        fog.LightIntensity = settings.volumetric_light_intensity;
        fog.AmbientIntensity = settings.volumetric_ambient_intensity;
        fog.NoiseFrame = noiseFrame;
        return fog;
    }

    ShadowSettings BuildShadowSettings(const ProjectLightingSettings& settings, uint32_t noiseFrame)
    {
        ShadowSettings shadow;
        shadow.ShadowBias = settings.shadow_bias;
        shadow.AmbientMultiplier = settings.ambient_multiplier;
        shadow.SkyLightIntensity = settings.sky_light_intensity;
        shadow.ReflectionIntensity = settings.reflection_intensity;
        shadow.AmbientColor = glm::vec4(settings.ambient_color[0], settings.ambient_color[1], settings.ambient_color[2], 1.0f);
        shadow.NoiseFrame = noiseFrame;
        return shadow;
    }

    CrossHatchConstants BuildCrossHatchConstants(const ProjectCrossHatchSettings& settings)
    {
        // smoothstep is undefined for equal edges and pow(0, 0) is NaN, so keep those away from 0
        constexpr float kMinSmoothness = 0.001f;

        CrossHatchConstants c;
        c.InkColor = glm::vec4(settings.ink_color[0], settings.ink_color[1], settings.ink_color[2], glm::clamp(settings.hatch_opacity, 0.0f, 1.0f));

        c.LitScale = settings.lit_scale;
        c.LitThickness = settings.lit_thickness;
        c.LitDensity = settings.lit_density;
        c.LitAngle = settings.lit_angle;

        c.DetailScale = settings.detail_scale;
        c.DetailDensity = settings.detail_density;
        c.DetailAngle = settings.detail_angle;
        c.DetailThickness = settings.detail_thickness;

        c.ShadowScale = settings.shadow_scale;
        c.ShadowDensity = settings.shadow_density;
        c.ShadowAngle = settings.shadow_angle;
        c.ShadowThickness = settings.shadow_thickness;

        c.LineSmoothness = glm::max(settings.line_smoothness, kMinSmoothness);
        c.ShadowSmoothness = glm::max(settings.shadow_smoothness, kMinSmoothness);
        c.ShadowCoreBias = glm::max(settings.shadow_core_bias, 0.01f);
        c.ShadowIntensity = settings.shadow_intensity;

        c.TexScale = settings.texture_scale;
        c.TexPriority = settings.texture_priority;
        c.TexBlackThreshold = settings.texture_black_threshold;
        c.TexSmoothness = glm::max(settings.texture_smoothness, kMinSmoothness);

        c.TexRotation = settings.texture_rotation;
        c.TexThickness = settings.texture_thickness;
        c.TexLightBias = settings.texture_light_bias;
        c.SpecularThreshold = settings.specular_threshold;

        c.OutlineWidth = settings.outline_width;
        c.OutlineMode = settings.outline_from_origin ? 1.0f : 0.0f;
        c.OutlineScaleWithDistance = settings.outline_constant_screen_width ? 1.0f : 0.0f;

        c.HatchMode = glm::clamp(settings.mode, 0, 2);
        c.InnerEdges = settings.inner_edges ? 1 : 0;
        c.SpecularEnabled = settings.specular_enabled ? 1 : 0;
        return c;
    }

}
