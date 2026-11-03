#include "SceneConstants.hpp"
#include <chrono>

namespace Diligent {

    FogConstants BuildFogConstants(const ProjectFogSettings& settings)
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
        return fog;
    }

    ShadowSettings BuildShadowSettings(const ProjectLightingSettings& settings)
    {
        ShadowSettings shadow;
        shadow.ShadowBias = settings.shadow_bias;
        shadow.AmbientMultiplier = settings.ambient_multiplier;
        shadow.SkyLightIntensity = settings.sky_light_intensity;
        shadow.ReflectionIntensity = settings.reflection_intensity;
        shadow.AmbientColor = glm::vec4(settings.ambient_color[0], settings.ambient_color[1], settings.ambient_color[2], 1.0f);
        return shadow;
    }

}
