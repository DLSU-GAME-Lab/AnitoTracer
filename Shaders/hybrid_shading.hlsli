#ifndef HYBRID_SHADING_HLSLI
#define HYBRID_SHADING_HLSLI

// Ray-traced shading shared by the forward Hybrid pass (hybrid_ps.hlsl) and the
// Deferred Hybrid lighting pass (DeferredHybrid/lighting_ps.hlsl), so both produce the same image.
// Requires light_struct.hlsli, pbr_defs.hlsli and fog.hlsli to be included first.

#include "rt_shadows.hlsli"
#include "Skybox/sky_common.hlsli"

// Sky lighting, filtered from the sky by SkyEnvironment when it loads (black 1x1 fallbacks until then).
// g_EnvIrradiance: the sky averaged over the hemisphere around each direction, for diffuse light.
// g_EnvSpecular:   the sky blurred a little more in each mip, for reflections. Mip 0 is a mirror, the last mip the roughest surface.
TextureCube g_EnvIrradiance;
TextureCube g_EnvSpecular;
SamplerState g_EnvMap_sampler;

// ------------------------------------------------------------------
// Ray budget / quality knobs (shadow ray counts live in rt_shadows.hlsli)
// ------------------------------------------------------------------
static const int AO_SAMPLES = 6;                // Cosine-weighted occlusion rays
static const float AO_RADIUS = 1.0;             // World units

// Bits of g_ReflectionFlags, from the project's reflection settings (mirror ReflectionFlags in PipelineDefs.hpp).
// The roughness limit and ray distance are settings too: g_ReflectionMaxRoughness and g_ReflectionMaxDistance.

static const uint REFLECTION_TRACE = 1u;        // Trace reflection rays; without it surfaces reflect only the sky
static const uint REFLECTION_SHADE_HITS = 2u;   // Light and texture what a ray hits; without it a hit only blocks the sky
static const uint REFLECTION_HIT_SHADOWS = 4u;  // Trace shadow rays from what a ray hits
static const uint REFLECTION_GLOSSY = 8u;       // Spread rays over rough surfaces' highlight (only while TAA averages frames)

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
// Diffuse sky light arriving at a surface facing N
float3 SampleEnvIrradiance(float3 N)
{
    return g_EnvIrradiance.SampleLevel(g_EnvMap_sampler, RotateSkyDirection(N, g_SkyRotation), 0.0).rgb;
}

// Sky reflected along R by a surface of the given roughness: each mip was filtered for one roughness, 0 in the first to 1 in the last
float3 SampleEnvSpecular(float3 R, float roughness)
{
    uint width, height, mips;
    g_EnvSpecular.GetDimensions(0, width, height, mips);

    float mip = roughness * max(float(mips) - 1.0, 0.0);
    return g_EnvSpecular.SampleLevel(g_EnvMap_sampler, RotateSkyDirection(R, g_SkyRotation), mip).rgb;
}

// Shading of ray hits, for reflections that show the scene. Uses the sky lighting above.
#ifdef RT_SCENE_SHADING
#include "rt_scene.hlsli"
#endif

// Direction to trace a reflection ray in. A mirror reflects along R alone, but a rough surface gathers light from
// a whole lobe of directions around it. With REFLECTION_GLOSSY each frame picks one direction out of that lobe (a
// GGX half vector, then reflecting the view direction about it), and TAA averages the frames into a blurred reflection.
// Without TAA the picks would never average out and only show as grain, so the ray stays on R.
float3 ReflectionRayDirection(float3 N, float3 geoN, float3 V, float3 R, float roughness, float2 noise)
{
    if ((g_ReflectionFlags & REFLECTION_GLOSSY) == 0u || g_NoiseFrame == 0u)
    {
        return R;
    }

    float a = roughness * roughness;
    // The lobe's far tail holds little light but most of the noise, so the outermost fifth is left out
    float u = noise.y * 0.8;
    float cosTheta = sqrt((1.0 - u) / (1.0 + (a * a - 1.0) * u));
    float sinTheta = sqrt(saturate(1.0 - cosTheta * cosTheta));
    float phi = noise.x * 2.0 * PI;

    float3 t, b;
    BuildBasis(N, t, b);
    float3 H = normalize((t * cos(phi) + b * sin(phi)) * sinTheta + N * cosTheta);

    // A direction that ends up below the surface would start inside it
    float3 dir = reflect(-V, H);
    return dot(dir, geoN) > 0.001 ? dir : R;
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
    float PixelSpread; // Angle one screen pixel covers, in radians. Tells a reflection ray how wide its footprint is
                       // when it reads a texture (see SampleSceneTexture); 0 always reads the sharpest mip.

    // Per-pixel GI (DiffuseGIPass), when the pipeline has it. With UseIndirectLight the ambient term comes from here
    // and no ambient occlusion rays are traced; without it the surface is lit by "Sky + AO".
    bool UseIndirectLight;
    float4 IndirectLight; // rgb: light arriving from other surfaces and the sky, averaged over the hemisphere
                          // a: how open the surroundings are, 0 enclosed .. 1 open
};

// Direct light with ray-traced soft shadows, indirect light (per-pixel GI or sky light with ray-traced AO),
// ray-traced reflections, then fog
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
    // Each light and effect gets its own shift of it (index k: dir lights 0-3, point lights 4-11, AO 12,
    // reflection ray 13, lights at the reflection's hit 14 onward)
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

    // 3. Indirect light
    float3 kD = (1.0 - specularColor) * (1.0 - metallic);
    float occlusion;        // How much of the surroundings is open, for the reflections below
    float3 irradiance;      // Average radiance arriving over the hemisphere
    float3 ambientDiffuse;
    if (s.UseIndirectLight)
    {
        // Per-pixel GI: the light was gathered by rays, so what blocks it is already accounted for.
        // Only the material's AO map is applied, for crevices smaller than the rays resolve.
        irradiance = s.IndirectLight.rgb;
        occlusion = min(s.AO, s.IndirectLight.a);
        ambientDiffuse = irradiance * s.Albedo * kD * s.AO;
    }
    else
    {
        // Sky + AO: flat ambient color plus skybox light, both occluded by ray-traced AO
        float3 bentNormal;
        float rtao = TraceAmbientOcclusion(s.WorldPos, geoN, s.PositionError, N, ShiftNoise(noise, 4 + 8), bentNormal);
        occlusion = min(s.AO, rtao);

        irradiance = g_AmbientColor.rgb + SampleEnvIrradiance(bentNormal) * g_SkyLightIntensity;
        ambientDiffuse = irradiance * s.Albedo * kD * occlusion;
    }

    // 4. Reflections: the sky along the mirror direction, replaced by the scene where a reflection ray hits it
    float3 R = reflect(-V, N);
    float specOcclusion = SpecularOcclusion(NdotV, occlusion, roughness);

    // Normal maps can point reflections into the surface; fade those out
    float horizon = saturate(1.0 + dot(R, geoN));
    horizon *= horizon;

    float3 envSpecular = SampleEnvSpecular(R, roughness) * g_SkyLightIntensity;
    if ((g_ReflectionFlags & REFLECTION_TRACE) != 0u && roughness < g_ReflectionMaxRoughness && g_ReflectionIntensity > 0.0)
    {
        // Rays fade out toward the roughness limit, where one ray says little about such a wide highlight
        float traceWeight = 1.0 - smoothstep(g_ReflectionMaxRoughness * 0.5, g_ReflectionMaxRoughness, roughness);
        // Stand-in for something the ray hit but can't shade: a dim bounce of the light around this surface
        float3 unshadedRadiance = irradiance * 0.25 * occlusion;

        bool shadeHits = false;
#ifdef RT_SCENE_SHADING
        shadeHits = (g_ReflectionFlags & REFLECTION_SHADE_HITS) != 0u;
        if (shadeHits)
        {
            float3 rayDir = ReflectionRayDirection(N, geoN, V, R, roughness, ShiftNoise(noise, 13));
            float3 rayOrigin = s.WorldPos + geoN * (g_ShadowBias + s.PositionError);

            // A miss keeps the sky filtered for this roughness, which has no noise; a hit shows the object, lit
            SceneHit hit;
            if (TraceSceneRay(rayOrigin, rayDir, g_ReflectionMaxDistance, hit))
            {
                float3 hitRadiance = unshadedRadiance;
                if (hit.Shadeable)
                {
                    // The pixel's footprint keeps widening along the reflected ray, and a rough reflector widens it further
                    float viewDistance = length(g_CameraPos.xyz - s.WorldPos);
                    float coneWidth = s.PixelSpread * (viewDistance + hit.Distance) + 0.5 * roughness * roughness * hit.Distance;
                    uint hitFlags = HIT_SPECULAR;
                    if ((g_ReflectionFlags & REFLECTION_HIT_SHADOWS) != 0u) hitFlags |= HIT_SHADOWS;
                    // With per-pixel GI, the reflected object gets the indirect light gathered at this pixel rather
                    // than open sky: it is usually close by, and a room without windows then stays dark in mirrors too
                    hitFlags |= s.UseIndirectLight ? HIT_AMBIENT_GIVEN : HIT_AMBIENT_SKY;
                    hitRadiance = ShadeSceneHit(hit, rayDir, coneWidth, ShiftNoise(noise, 14), hitFlags, irradiance);

                }
                envSpecular = lerp(envSpecular, hitRadiance, traceWeight);
            }

            // The ray settled what is visible along the reflection, so the AO-based guess only remains where rays fade out
            specOcclusion = lerp(specOcclusion, 1.0, traceWeight);
        }
#endif
        if (!shadeHits)
        {
            // Visibility only: geometry in the way hides the sky but isn't shown
            float reflectionVisibility = TraceShadowRay(s.WorldPos, geoN, s.PositionError, R, g_ReflectionMaxDistance);
            envSpecular = lerp(envSpecular, lerp(unshadedRadiance, envSpecular, reflectionVisibility), traceWeight);
        }
    }
    float3 ambientSpecular = envSpecular * specularColor * energyCompensation * specOcclusion * horizon * g_ReflectionIntensity;


    float3 ambient = (ambientDiffuse + ambientSpecular) * g_AmbientMultiplier;
    float3 color = ambient + Lo + s.Emissive;
    return ApplyFog(color, s.WorldPos, pixelPos);
}

#endif // HYBRID_SHADING_HLSLI
