#ifndef HYBRID_SHADING_HLSLI
#define HYBRID_SHADING_HLSLI

// Ray-traced shading shared by the forward Hybrid pass (hybrid_ps.hlsl) and the Deferred Hybrid passes
// (DeferredHybrid/*.hlsl), so both produce the same image.
//
// It comes in two halves:
//   tracing    TraceLightSlot, TraceAmbientOcclusion, TraceHybridReflection: what the rays from a surface find
//   composing  ComposeHybridSurface: the surface's color, from its material and those results (HybridRays)
// The forward pass does both at once for every pixel it draws (ShadeHybridSurface), with several rays per light.
// Deferred Hybrid traces in passes of their own with one ray each, cleans the results up over time and space
// (SignalDenoiser) and only composes in its lighting pass.
//
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
static const int AO_SAMPLES = 6;                // Cosine-weighted occlusion rays of the forward pass
static const float AO_RADIUS = 1.0;             // World units

// Lights are numbered in one list for the ray passes: the directional lights first, then the point lights
static const int MAX_LIGHT_SLOTS = 12;          // 4 directional + 8 point, see light_struct.hlsli

// Bits of g_ReflectionFlags, from the project's reflection settings (mirror ReflectionFlags in PipelineDefs.hpp).
// The roughness limit and ray distance are settings too: g_ReflectionMaxRoughness and g_ReflectionMaxDistance.

static const uint REFLECTION_TRACE = 1u;        // Trace reflection rays; without it surfaces reflect only the sky
static const uint REFLECTION_SHADE_HITS = 2u;   // Light and texture what a ray hits; without it a hit only blocks the sky
static const uint REFLECTION_HIT_SHADOWS = 4u;  // Trace shadow rays from what a ray hits
static const uint REFLECTION_GLOSSY = 8u;       // Spread rays over rough surfaces' highlight (only while frames are averaged)

// Ray-traced ambient occlusion with `samples` rays. Returns the share of them that got away, and in openDirection
// the average direction of those (not normalized: it shortens as more rays are blocked).
float TraceAmbientOcclusion(float3 worldPos, float3 geoN, float positionError, float3 N, float2 noise, int samples, out float3 openDirection)
{
    float3 t, b;
    BuildBasis(N, t, b);

    float visibility = 0.0;
    openDirection = float3(0.0, 0.0, 0.0);
    for (int s = 0; s < samples; ++s)
    {
        // Disk -> hemisphere projection gives a cosine-weighted distribution (Malley's method)
        float2 d = VogelDisk(s, samples, noise.x * 2.0 * PI, noise.y);
        float3 dir = t * d.x + b * d.y + N * sqrt(saturate(1.0 - dot(d, d)));

        // Normal maps can tilt samples below the real surface; mirror them back above it
        float below = dot(dir, geoN);
        if (below < 0.0)
        {
            dir -= 2.0 * below * geoN;
        }

        float v = TraceShadowRay(worldPos, geoN, positionError, dir, AO_RADIUS);
        visibility += v;
        openDirection += dir * v;
    }

    openDirection /= float(samples);
    return visibility / float(samples);
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
// GGX half vector, then reflecting the view direction about it), and averaging the frames (the denoiser in Deferred
// Hybrid, TAA elsewhere) turns them into a blurred reflection.
// With nothing to average them the picks would only show as grain, so the ray stays on R.
float3 ReflectionRayDirection(float3 N, float3 geoN, float3 V, float3 R, float roughness, float2 noise)
{
    if ((g_ReflectionFlags & REFLECTION_GLOSSY) == 0u || !RaysAveraged())
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

// What the rays from a surface found. Traced on the spot by the forward pass, read from the denoised signal
// textures by the Deferred Hybrid lighting pass.
struct HybridRays
{
    float LightVisibility[MAX_LIGHT_SLOTS]; // Per light slot: 0 in shadow .. 1 fully lit
    // Ambient occlusion, for Sky + AO (not read with UseIndirectLight)
    float AmbientVisibility;    // 0 enclosed .. 1 open
    float3 OpenDirection;       // Average direction of the occlusion rays that got away, see TraceAmbientOcclusion
    // The light arriving along the reflection, see TraceHybridReflection (not read where UsesReflectionRay is false)
    float3 Reflection;
};

int LightSlotCount()
{
    return g_NumDirLights + g_NumPointLights;
}

// How much of a light reaches the surface, from `samples` shadow rays spread over the light's area.
// Returns 1 without tracing where the light can't reach anyway (facing away, out of range, an empty slot):
// the light's own shading is 0 there, and 1 keeps a blur over the result from darkening the lit side next to it.
// noise: this pixel's RayNoise / HashNoise; each light shifts it differently.
float TraceLightSlot(HybridSurface s, int slot, float2 noise, int samples)
{
    if (slot < g_NumDirLights)
    {
        float3 L = normalize(-g_DirLights[slot].Direction.xyz);
        if (dot(s.N, L) <= 0.0)
            return 1.0;

        return TraceDirectionalShadow(s.WorldPos, s.GeoN, s.PositionError, L, ShiftNoise(noise, slot), samples);
    }

    int j = slot - g_NumDirLights;
    if (j >= g_NumPointLights)
        return 1.0;

    float3 lightVec = g_PointLights[j].Position.xyz - s.WorldPos;
    float dist = length(lightVec);
    if (dist >= g_PointLights[j].Range)
        return 1.0;

    float3 L = lightVec / dist;
    if (dot(s.N, L) <= 0.0)
        return 1.0;

    return TracePointShadow(s.WorldPos, s.GeoN, s.PositionError, g_PointLights[j].Position.xyz, L, ShiftNoise(noise, 4 + j), samples);
}

// The indirect light at a surface, from per-pixel GI or from the sky dimmed by ambient occlusion
struct HybridAmbient
{
    float3 Irradiance;      // Average radiance arriving over the hemisphere
    float Occlusion;        // How much of the surroundings is open, for the reflections
    float DiffuseOcclusion; // What the diffuse part is dimmed by
};

HybridAmbient GetHybridAmbient(HybridSurface s, float ambientVisibility, float3 openDirection)
{
    HybridAmbient ambient;
    if (s.UseIndirectLight)
    {
        // Per-pixel GI: the light was gathered by rays, so what blocks it is already accounted for.
        // Only the material's AO map is applied, for crevices smaller than the rays resolve.
        ambient.Irradiance = s.IndirectLight.rgb;
        ambient.Occlusion = min(s.AO, s.IndirectLight.a);
        ambient.DiffuseOcclusion = s.AO;
    }
    else
    {
        // Sky + AO: flat ambient color plus skybox light from the open side (the bent normal), both occluded by ray-traced AO
        float3 bentNormal = dot(openDirection, openDirection) > 0.000001 ? normalize(openDirection) : s.N;
        ambient.Occlusion = min(s.AO, ambientVisibility);
        ambient.Irradiance = g_AmbientColor.rgb + SampleEnvIrradiance(bentNormal) * g_SkyLightIntensity;
        ambient.DiffuseOcclusion = ambient.Occlusion;
    }
    return ambient;
}

// True where a reflection ray is traced for a surface of this roughness; rougher ones reflect the filtered sky only
bool UsesReflectionRay(float roughness)
{
    return (g_ReflectionFlags & REFLECTION_TRACE) != 0u && roughness < g_ReflectionMaxRoughness && g_ReflectionIntensity > 0.0;
}

// True when reflection rays show what they hit, lit and textured, rather than only finding out that something is there
bool ReflectionShadesHits()
{
#ifdef RT_SCENE_SHADING
    return (g_ReflectionFlags & REFLECTION_SHADE_HITS) != 0u;
#else
    return false;
#endif
}

// One reflection ray from a surface. Call only where UsesReflectionRay(s.Roughness).
// rgb: the light arriving along the reflection: the object the ray hit, lit, or the sky filtered for this roughness
//      where it hit nothing (which has no noise).
// a:   how far away the hit was, in world units; g_ReflectionMaxDistance for the sky. The denoiser sizes its blur with it.
// noise: this pixel's RayNoise / HashNoise.
float4 TraceHybridReflection(HybridSurface s, HybridAmbient ambient, float2 noise)
{
    float3 V = normalize(g_CameraPos.xyz - s.WorldPos);
    float3 R = reflect(-V, s.N);

    float3 envSpecular = SampleEnvSpecular(R, s.Roughness) * g_SkyLightIntensity;
    // Stand-in for something the ray hit but can't shade: a dim bounce of the light around this surface
    float3 unshadedRadiance = ambient.Irradiance * 0.25 * ambient.Occlusion;

#ifdef RT_SCENE_SHADING
    if ((g_ReflectionFlags & REFLECTION_SHADE_HITS) != 0u)
    {
        float3 rayDir = ReflectionRayDirection(s.N, s.GeoN, V, R, s.Roughness, ShiftNoise(noise, 13));
        float3 rayOrigin = s.WorldPos + s.GeoN * (g_ShadowBias + s.PositionError);

        SceneHit hit;
        if (!TraceSceneRay(rayOrigin, rayDir, g_ReflectionMaxDistance, hit))
        {
            return float4(envSpecular, g_ReflectionMaxDistance);
        }

        float3 hitRadiance = unshadedRadiance;
        if (hit.Shadeable)
        {
            // The pixel's footprint keeps widening along the reflected ray, and a rough reflector widens it further
            float viewDistance = length(g_CameraPos.xyz - s.WorldPos);
            float coneWidth = s.PixelSpread * (viewDistance + hit.Distance) + 0.5 * s.Roughness * s.Roughness * hit.Distance;
            uint hitFlags = HIT_SPECULAR;
            if ((g_ReflectionFlags & REFLECTION_HIT_SHADOWS) != 0u) hitFlags |= HIT_SHADOWS;
            // With per-pixel GI, the reflected object gets the indirect light gathered at this pixel rather
            // than open sky: it is usually close by, and a room without windows then stays dark in mirrors too
            hitFlags |= s.UseIndirectLight ? HIT_AMBIENT_GIVEN : HIT_AMBIENT_SKY;
            hitRadiance = ShadeSceneHit(hit, rayDir, coneWidth, ShiftNoise(noise, 14), hitFlags, ambient.Irradiance);
        }
        return float4(hitRadiance, hit.Distance);
    }
#endif

    // Visibility only: geometry in the way hides the sky but isn't shown. How far away it is stays unknown.
    float reflectionVisibility = TraceShadowRay(s.WorldPos, s.GeoN, s.PositionError, R, g_ReflectionMaxDistance);
    return float4(lerp(unshadedRadiance, envSpecular, reflectionVisibility), reflectionVisibility * g_ReflectionMaxDistance);
}

// The surface's color from its material and what its rays found: direct light with ray-traced shadows, indirect
// light (per-pixel GI, or sky light with ray-traced AO), reflections, then fog
float3 ComposeHybridSurface(HybridSurface s, HybridRays rays, float2 pixelPos)
{
    float3 N = s.N;
    float3 geoN = s.GeoN;
    float metallic = s.Metallic;
    float roughness = s.Roughness;

    float3 V = normalize(g_CameraPos.xyz - s.WorldPos);
    float NdotV = saturate(dot(N, V)) + 0.0001;
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), s.Albedo, metallic);
    float3 Lo = float3(0.0, 0.0, 0.0);

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

        float shadowFactor = rays.LightVisibility[i];
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

            float shadowFactor = rays.LightVisibility[g_NumDirLights + j];

            float attenuation = max(1.0 - (dist / g_PointLights[j].Range), 0.0);
            attenuation *= attenuation;
            float3 radiance = g_PointLights[j].Color.rgb * g_PointLights[j].Color.a * attenuation;

            Lo += ComputeLightRadiance(N, V, L, radiance, s.Albedo, metallic, roughness, F0, shadowFactor, energyCompensation);
        }
    }

    // 3. Indirect light
    HybridAmbient indirect = GetHybridAmbient(s, rays.AmbientVisibility, rays.OpenDirection);
    float3 kD = (1.0 - specularColor) * (1.0 - metallic);
    float3 ambientDiffuse = indirect.Irradiance * s.Albedo * kD * indirect.DiffuseOcclusion;

    // 4. Reflections: the sky along the mirror direction, replaced by what the reflection ray found
    float3 R = reflect(-V, N);
    float specOcclusion = SpecularOcclusion(NdotV, indirect.Occlusion, roughness);

    // Normal maps can point reflections into the surface; fade those out
    float horizon = saturate(1.0 + dot(R, geoN));
    horizon *= horizon;

    float3 envSpecular = SampleEnvSpecular(R, roughness) * g_SkyLightIntensity;
    if (UsesReflectionRay(roughness))
    {
        // Rays fade out toward the roughness limit, where one ray says little about such a wide highlight
        float traceWeight = 1.0 - smoothstep(g_ReflectionMaxRoughness * 0.5, g_ReflectionMaxRoughness, roughness);
        envSpecular = lerp(envSpecular, rays.Reflection, traceWeight);

        // The ray settled what is visible along the reflection, so the AO-based guess only remains where rays fade out
        if (ReflectionShadesHits())
        {
            specOcclusion = lerp(specOcclusion, 1.0, traceWeight);
        }
    }
    float3 ambientSpecular = envSpecular * specularColor * energyCompensation * specOcclusion * horizon * g_ReflectionIntensity;


    float3 ambient = (ambientDiffuse + ambientSpecular) * g_AmbientMultiplier;
    float3 color = ambient + Lo + s.Emissive;
    return ApplyFog(color, s.WorldPos, pixelPos);
}

// Traces and composes in one go, for a pass with nothing to clean up single rays: several rays per light and for
// the ambient occlusion, their pattern rotated per pixel (and per frame under TAA)
float3 ShadeHybridSurface(HybridSurface s, float2 pixelPos)
{
    // Each light and effect gets its own shift of the pixel's noise (index k: dir lights 0-3, point lights 4-11,
    // AO 12, reflection ray 13, lights at the reflection's hit 14 onward)
    float2 noise = RayNoise(pixelPos);

    HybridRays rays;
    int slotCount = LightSlotCount();
    for (int slot = 0; slot < MAX_LIGHT_SLOTS; ++slot)
    {
        rays.LightVisibility[slot] = slot < slotCount ? TraceLightSlot(s, slot, noise, SHADOW_SAMPLES) : 1.0;
    }

    rays.AmbientVisibility = 1.0;
    rays.OpenDirection = s.N;
    if (!s.UseIndirectLight)
    {
        rays.AmbientVisibility = TraceAmbientOcclusion(s.WorldPos, s.GeoN, s.PositionError, s.N, ShiftNoise(noise, 4 + 8), AO_SAMPLES, rays.OpenDirection);
    }

    rays.Reflection = float3(0.0, 0.0, 0.0);
    if (UsesReflectionRay(s.Roughness))
    {
        HybridAmbient ambient = GetHybridAmbient(s, rays.AmbientVisibility, rays.OpenDirection);
        rays.Reflection = TraceHybridReflection(s, ambient, noise).rgb;
    }

    return ComposeHybridSurface(s, rays, pixelPos);
}

#endif // HYBRID_SHADING_HLSLI
