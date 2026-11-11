#ifndef HYBRID_SHADING_HLSLI
#define HYBRID_SHADING_HLSLI

// Ray-traced shading shared by the forward Hybrid pass (hybrid_ps.hlsl) and the
// Deferred Hybrid lighting pass (DeferredHybrid/lighting_ps.hlsl), so both produce the same image.
// Requires light_struct.hlsli, pbr_defs.hlsli and fog.hlsli to be included first.

#include "rt_shadows.hlsli"

// Skybox cubemap, used as the image-based light source (black 1x1 fallback when no skybox is loaded)
TextureCube g_EnvMap;
SamplerState g_EnvMap_sampler;

// ------------------------------------------------------------------
// Ray budget / quality knobs (shadow ray counts live in rt_shadows.hlsli)
// ------------------------------------------------------------------
static const int AO_SAMPLES = 6;                // Cosine-weighted occlusion rays
static const float AO_RADIUS = 1.0;             // World units
static const float REFLECTION_MAX_ROUGHNESS = 0.6; // Above this, specular occlusion from AO is enough
static const float REFLECTION_RAY_DISTANCE = 100.0;

// Ray-traced ambient occlusion. Also returns the bent normal (average unoccluded direction)
float TraceAmbientOcclusion(float3 worldPos, float3 geoN, float positionError, float3 N, float2 noise, out float3 bentNormal)
{
    float3 t, b;
    BuildBasis(N, t, b);

    float visibility = 0.0;
    float3 openDir = float3(0.0, 0.0, 0.0);
    for (int s = 0; s < AO_SAMPLES; ++s)
    {
        // Disk -> hemisphere projection gives a cosine-weighted distribution (Malley's method)
        float2 d = VogelDisk(s, AO_SAMPLES, noise.x * 2.0 * PI, noise.y);
        float3 dir = t * d.x + b * d.y + N * sqrt(saturate(1.0 - dot(d, d)));

        // Normal maps can tilt samples below the real surface; mirror them back above it
        float below = dot(dir, geoN);
        if (below < 0.0)
        {
            dir -= 2.0 * below * geoN;
        }

        float v = TraceShadowRay(worldPos, geoN, positionError, dir, AO_RADIUS);
        visibility += v;
        openDir += dir * v;
    }

    bentNormal = dot(openDir, openDir) > 0.0001 ? normalize(openDir) : N;
    return visibility / AO_SAMPLES;
}

// Specular occlusion from ambient occlusion (Lagarde & de Rousiers, "Moving Frostbite to PBR")
float SpecularOcclusion(float NdotV, float ao, float roughness)
{
    return saturate(pow(NdotV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao);
}

// ------------------------------------------------------------------
// Image based lighting from the skybox
// ------------------------------------------------------------------
float GetEnvMipCount()
{
    uint width, height, mips;
    g_EnvMap.GetDimensions(0, width, height, mips);
    return (float)mips;
}

// Cheap irradiance: a few taps of a blurry mip around the normal
float3 SampleEnvIrradiance(float3 N, float mipCount)
{
    float mip = max(mipCount - 3.0, 0.0);

    float3 t, b;
    BuildBasis(N, t, b);

    float3 irradiance = g_EnvMap.SampleLevel(g_EnvMap_sampler, N, mip).rgb * 2.0;
    irradiance += g_EnvMap.SampleLevel(g_EnvMap_sampler, normalize(N + t), mip).rgb;
    irradiance += g_EnvMap.SampleLevel(g_EnvMap_sampler, normalize(N - t), mip).rgb;
    irradiance += g_EnvMap.SampleLevel(g_EnvMap_sampler, normalize(N + b), mip).rgb;
    irradiance += g_EnvMap.SampleLevel(g_EnvMap_sampler, normalize(N - b), mip).rgb;
    return irradiance / 6.0;
}

// Rougher surfaces read blurrier mips
float3 SampleEnvSpecular(float3 R, float roughness, float mipCount)
{
    float mip = roughness * max(mipCount - 1.0, 0.0);
    return g_EnvMap.SampleLevel(g_EnvMap_sampler, R, mip).rgb;
}


// ------------------------------------------------------------------
// Surface shading
// ------------------------------------------------------------------

// Everything the shading needs about one visible surface point, after material sampling
struct HybridSurface
{
    float3 WorldPos;
    float3 N;        // Shading normal (normal mapped)
    float3 GeoN;     // Unperturbed geometric normal, used strictly for ray origin offsets
    float PositionError; // World units WorldPos may be off the surface; 0 when interpolated, > 0 when rebuilt from depth
    float3 Albedo;
    float Metallic;
    float Roughness; // Already clamped to >= 0.05
    float AO;        // Material AO map
    float3 Emissive;
};

// Direct light with ray-traced soft shadows, RTAO-occluded ambient and sky reflections, then fog
float3 ShadeHybridSurface(HybridSurface s, float2 pixelPos)
{
    float3 N = s.N;
    float3 geoN = s.GeoN;
    float metallic = s.Metallic;
    float roughness = s.Roughness;

    float3 V = normalize(g_CameraPos.xyz - s.WorldPos);
    float NdotV = saturate(dot(N, V)) + 0.0001;
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), s.Albedo, metallic);
    float3 Lo = float3(0.0, 0.0, 0.0);

    // Per-pixel rotation for all stochastic rays, so the sample patterns don't band; changes every frame under TAA.
    // Each light and the AO get their own shift of it (index k: dir lights 0-3, point lights 4-11, AO 12)
    float2 noise = RayNoise(pixelPos);

    // Single scattering GGX loses energy on rough surfaces; scale it back up (Fdez-Aguera 2019)
    float2 envBRDF = EnvBRDFApprox(NdotV, roughness);
    float3 specularColor = F0 * envBRDF.x + envBRDF.y;
    float3 energyCompensation = 1.0 + F0 * (1.0 / max(envBRDF.x + envBRDF.y, 0.0001) - 1.0);

    // 1. Directional Lights
    for (int i = 0; i < g_NumDirLights; ++i)
    {
        float3 L = normalize(-g_DirLights[i].Direction.xyz);
        if (dot(N, L) <= 0.0)
            continue;

        float shadowFactor = TraceDirectionalShadow(s.WorldPos, geoN, s.PositionError, L, ShiftNoise(noise, i));
        float3 radiance = g_DirLights[i].Color.rgb * g_DirLights[i].Color.a;

        Lo += ComputeLightRadiance(N, V, L, radiance, s.Albedo, metallic, roughness, F0, shadowFactor, energyCompensation);
    }

    // 2. Point Lights
    for (int j = 0; j < g_NumPointLights; ++j)
    {
        float3 lightVec = g_PointLights[j].Position.xyz - s.WorldPos;
        float dist = length(lightVec);

        if (dist < g_PointLights[j].Range)
        {
            float3 L = lightVec / dist;
            if (dot(N, L) <= 0.0)
                continue;

            float shadowFactor = TracePointShadow(s.WorldPos, geoN, s.PositionError, g_PointLights[j].Position.xyz, L, ShiftNoise(noise, 4 + j));

            float attenuation = max(1.0 - (dist / g_PointLights[j].Range), 0.0);
            attenuation *= attenuation;
            float3 radiance = g_PointLights[j].Color.rgb * g_PointLights[j].Color.a * attenuation;

            Lo += ComputeLightRadiance(N, V, L, radiance, s.Albedo, metallic, roughness, F0, shadowFactor, energyCompensation);
        }
    }

    // 3. Ambient: flat ambient color plus skybox light, both occluded by ray-traced AO
    float3 bentNormal;
    float rtao = TraceAmbientOcclusion(s.WorldPos, geoN, s.PositionError, N, ShiftNoise(noise, 4 + 8), bentNormal);
    float occlusion = min(s.AO, rtao);

    float envMipCount = GetEnvMipCount();
    float3 irradiance = g_AmbientColor.rgb + SampleEnvIrradiance(bentNormal, envMipCount) * g_SkyLightIntensity;
    float3 kD = (1.0 - specularColor) * (1.0 - metallic);
    float3 ambientDiffuse = irradiance * s.Albedo * kD * occlusion;

    // 4. Reflections of the skybox, blocked where scene geometry is in the way
    float3 R = reflect(-V, N);
    float specOcclusion = SpecularOcclusion(NdotV, occlusion, roughness);

    // Normal maps can point reflections into the surface; fade those out
    float horizon = saturate(1.0 + dot(R, geoN));
    specOcclusion *= horizon * horizon;

    float3 envSpecular = SampleEnvSpecular(R, roughness, envMipCount) * g_SkyLightIntensity;
    if (roughness < REFLECTION_MAX_ROUGHNESS && g_ReflectionIntensity > 0.0)
    {
        // A reflection ray can't shade what it hits (no hit shading here), so stand in a dim
        // bounce of the local irradiance for the occluder instead of showing sky through it
        float reflectionVisibility = TraceShadowRay(s.WorldPos, geoN, s.PositionError, R, REFLECTION_RAY_DISTANCE);
        float3 occludedRadiance = irradiance * 0.25 * occlusion;
        float traceWeight = 1.0 - smoothstep(REFLECTION_MAX_ROUGHNESS * 0.5, REFLECTION_MAX_ROUGHNESS, roughness);
        envSpecular = lerp(envSpecular, lerp(occludedRadiance, envSpecular, reflectionVisibility), traceWeight);
    }
    float3 ambientSpecular = envSpecular * specularColor * energyCompensation * specOcclusion * g_ReflectionIntensity;

    float3 ambient = (ambientDiffuse + ambientSpecular) * g_AmbientMultiplier;
    float3 color = ambient + Lo + s.Emissive;
    return ApplyFog(color, s.WorldPos, pixelPos);
}

#endif // HYBRID_SHADING_HLSLI
