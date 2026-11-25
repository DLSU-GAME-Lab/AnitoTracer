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
#include "light_shading.hlsli"
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
        // Disk -> hemisphere projection gives a cosine-weighted distribution (Malley's method). Normal maps can tilt
        // samples below the real surface; those are mirrored back above it
        float2 d = VogelDisk(s, samples, noise.x * 2.0 * PI, noise.y);
        float3 dir = KeepAboveSurface(CosineHemisphereDirection(N, t, b, d), geoN);

        float v = TraceOcclusionRay(worldPos, geoN, positionError, dir, AO_RADIUS);
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
    // The color glass in the way gives the surface's direct light, as one color for all its lights (white without any
    // glass). Each light's tint counts as much as the light does at the surface.
    float3 DirectTint;
};

int LightSlotCount()
{
    return g_NumDirLights + g_NumPointLights;
}

// How much of a light reaches the surface, from `samples` shadow rays spread over the light's area.
// Returns 1 without tracing where the light can't reach anyway (facing away, out of range, an empty slot):
// the light's own shading is 0 there, and 1 keeps a blur over the result from darkening the lit side next to it.
// noise: this pixel's RayNoise / HashNoise; each light shifts it differently.
// Glass in the way lets part of the light through: how much of it counts in the result (as its brightness), and its
// color goes in `chroma`: white where nothing tints the light.
float TraceLightSlot(HybridSurface s, int slot, float2 noise, int samples, out float3 chroma)
{
    chroma = float3(1.0, 1.0, 1.0);
    float3 light;

    if (slot < g_NumDirLights)
    {
        float3 L = normalize(-g_DirLights[slot].Direction.xyz);
        if (dot(s.N, L) <= 0.0)
            return 1.0;

        light = TraceDirectionalShadowRGB(s.WorldPos, s.GeoN, s.PositionError, L, tan(g_DirLights[slot].Direction.w), ShiftNoise(noise, slot), samples);
    }
    else
    {
        int j = slot - g_NumDirLights;
        if (j >= g_NumPointLights)
            return 1.0;

        float3 L;
        float dist;
        if (!LocalLightReaches(j, s.WorldPos, L, dist))
            return 1.0;

        float2 lightNoise = ShiftNoise(noise, 4 + j);
        if (g_PointLights[j].RectArea > 0.0)
        {
            // A panel lights a surface whose normal turns away from its center too, as long as part of it is in view:
            // the rays decide, and the points that are below the horizon count as lit
            light = TraceRectShadowRGB(s.WorldPos, s.GeoN, s.PositionError, g_PointLights[j].Position.xyz,
                g_PointLights[j].AxisX.xyz, g_PointLights[j].AxisY.xyz, lightNoise, samples);
        }
        else
        {
            if (dot(s.N, L) <= 0.0)
                return 1.0;

            light = TracePointShadowRGB(s.WorldPos, s.GeoN, s.PositionError, g_PointLights[j].Position.xyz, g_PointLights[j].Position.w, L, lightNoise, samples);
        }
    }

    float visibility = dot(light, LUMINANCE_WEIGHTS);
    if (visibility > 0.0001)
    {
        chroma = light / visibility;
    }
    return visibility;
}

float TraceLightSlot(HybridSurface s, int slot, float2 noise, int samples)
{
    float3 chroma;
    return TraceLightSlot(s, slot, noise, samples, chroma);
}

// How much a light adds to the surface, roughly: its brightness there, before shadows. Decides how much its glass
// tint counts in the surface's single DirectTint.
float LightTintWeight(HybridSurface s, int slot)
{
    if (slot < g_NumDirLights)
    {
        float3 L = normalize(-g_DirLights[slot].Direction.xyz);
        return saturate(dot(s.N, L)) * dot(g_DirLights[slot].Color.rgb * g_DirLights[slot].Color.a, LUMINANCE_WEIGHTS);
    }

    int j = slot - g_NumDirLights;
    if (j >= g_NumPointLights)
        return 0.0;

    float3 L;
    float dist;
    float attenuation = LocalLightAttenuation(j, s.WorldPos, L, dist);
    return saturate(dot(s.N, L)) * dot(g_PointLights[j].Color.rgb * g_PointLights[j].Color.a, LUMINANCE_WEIGHTS) * attenuation;
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
float3 ComposeHybridSurfaceNoFog(HybridSurface s, HybridRays rays)
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

        Lo += ShadeRoundLight(N, V, L, tan(g_DirLights[i].Direction.w), radiance, s.Albedo, metallic, roughness, F0, shadowFactor, energyCompensation);
    }

    // 2. Point, spot and rectangular lights
    for (int j = 0; j < g_NumPointLights; ++j)
    {
        LocalLightSample light;
        if (!EvaluateLocalLight(j, s.WorldPos, N, V, light))
            continue;

        Lo += ShadeLocalLight(N, V, light, s.Albedo, metallic, roughness, F0, rays.LightVisibility[g_NumDirLights + j], energyCompensation);
    }

    // Light that came through glass has the glass's color
    Lo *= rays.DirectTint;

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
    return ambient + Lo + s.Emissive;
}

float3 ComposeHybridSurface(HybridSurface s, HybridRays rays, float2 pixelPos)
{
    return ApplyFog(ComposeHybridSurfaceNoFog(s, rays), s.WorldPos, pixelPos);
}

// Traces what the rays from a surface find, for a pass with nothing to clean up single rays: several rays per light and
// for the ambient occlusion, their pattern rotated per pixel (and per frame under TAA)
HybridRays TraceHybridRays(HybridSurface s, float2 pixelPos)
{
    // Each light and effect gets its own shift of the pixel's noise (index k: dir lights 0-3, point lights 4-11,
    // AO 12, reflection ray 13, lights at the reflection's hit 14 onward)
    float2 noise = RayNoise(pixelPos);

    HybridRays rays;
    int slotCount = LightSlotCount();
    float3 tintSum = float3(0.0, 0.0, 0.0);
    float tintWeight = 0.0;
    for (int slot = 0; slot < MAX_LIGHT_SLOTS; ++slot)
    {
        float visibility = 1.0;
        if (slot < slotCount)
        {
            float3 chroma;
            visibility = TraceLightSlot(s, slot, noise, SHADOW_SAMPLES, chroma);

            float weight = LightTintWeight(s, slot) * visibility;
            tintSum += chroma * weight;
            tintWeight += weight;
        }
        rays.LightVisibility[slot] = visibility;
    }
    rays.DirectTint = tintWeight > 0.00001 ? tintSum / tintWeight : float3(1.0, 1.0, 1.0);

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

    return rays;
}

// Traces and composes in one go
float3 ShadeHybridSurface(HybridSurface s, float2 pixelPos)
{
    return ComposeHybridSurface(s, TraceHybridRays(s, pixelPos), pixelPos);
}

// ------------------------------------------------------------------
// Glass
// ------------------------------------------------------------------

#ifdef RT_SCENE_SHADING
// What is seen through a pane of glass at a surface point: the view ray bends into the glass (index of refraction
// `ior`) and out of its far side, then goes on to whatever is behind: the scene, lit like a reflection's hit, or the sky.
// Rays go through other glass on the way, which tints what they find.
float3 TraceRefraction(HybridSurface s, float ior, float2 noise)
{
    float3 D = normalize(s.WorldPos - g_CameraPos.xyz);
    float3 N = dot(s.N, D) < 0.0 ? s.N : -s.N;

    // A closed shape (a ball, a slab) has a far side for the ray to leave through. A lone sheet has none, and is thin
    // enough that bending the view through it would only shift things for no reason: it goes straight through.
    float3 outOrigin = s.WorldPos + D * 0.002;
    float3 outDir = D;

    float3 inDir = refract(D, N, 1.0 / ior);
    if (dot(inDir, inDir) > 0.0)
    {
        float3 ignoredTint;
        SceneHit exitHit;
        if (TraceSceneRay(s.WorldPos + inDir * 0.002, inDir, 100.0, GLASS_SOLID, ignoredTint, exitHit)
            && exitHit.BackFacing && (exitHit.Material.Flags & SCENE_GEOMETRY_BLEND) != 0u)
        {
            float3 dir = inDir;
            for (int bounce = 0; bounce < 3; ++bounce)
            {
                // Leaving the glass bends the ray away from the normal. Past the critical angle there is nothing to
                // leave into: the ray turns back inside and heads for the next face.
                float3 leaving = refract(dir, exitHit.N, ior);
                if (dot(leaving, leaving) > 0.0)
                {
                    outDir = leaving;
                    outOrigin = exitHit.Position + outDir * 0.002;
                    break;
                }

                dir = reflect(dir, exitHit.N);
                if (!TraceSceneRay(exitHit.Position + dir * 0.002, dir, 100.0, GLASS_SOLID, ignoredTint, exitHit)
                    || !exitHit.BackFacing || (exitHit.Material.Flags & SCENE_GEOMETRY_BLEND) == 0u)
                {
                    break;
                }
            }
        }
    }

    float3 tint;
    SceneHit hit;
    float3 radiance;
    if (TraceSceneRay(outOrigin, outDir, g_ReflectionMaxDistance, GLASS_TINT, tint, hit))
    {
        // Not shadable (no data for its triangle): a dim stand-in
        radiance = g_AmbientColor.rgb * 0.5;
        if (hit.Shadeable)
        {
            float viewDistance = length(g_CameraPos.xyz - s.WorldPos);
            float coneWidth = s.PixelSpread * (viewDistance + hit.Distance);
            radiance = ShadeSceneHit(hit, outDir, coneWidth, ShiftNoise(noise, 14), HIT_SPECULAR | HIT_SHADOWS | HIT_AMBIENT_SKY, float3(0.0, 0.0, 0.0));
        }
    }
    else
    {
        radiance = SampleEnvSpecular(outDir, 0.0) * g_SkyLightIntensity;
    }
    return radiance * tint;
}
#endif

// A pane of glass: a blended material, drawn after the lit scene. s.Albedo is the glass's color, alpha how opaque it is,
// ior its index of refraction.
// Returns the color premultiplied by coverage, for blending over what is behind (ONE, INV_SRC_ALPHA):
//   its own reflections, highlights and emission, which no amount of transparency dims (a window still mirrors the sky),
//   its colour lit like any surface, as much of it as alpha says, and
//   what is behind it, through the glass: refracted and tinted when the scene can be traced from here, else left to the
//   blend, which shows the same view unbent.
float4 ShadeGlassSurface(HybridSurface s, float alpha, float ior, float2 pixelPos)
{
    HybridRays rays = TraceHybridRays(s, pixelPos);

    // Without a diffuse color, what the shading gives is what the surface reflects
    HybridSurface clear = s;
    clear.Albedo = float3(0.0, 0.0, 0.0);
    float3 reflected = ComposeHybridSurfaceNoFog(clear, rays);
    float3 body = max(ComposeHybridSurfaceNoFog(s, rays) - reflected, 0.0);

    float3 color = reflected + alpha * body;
    float coverage = alpha;

#ifdef RT_SCENE_SHADING
    if ((g_TransparencyFlags & TRANSPARENCY_REFRACTION) != 0u && ior > 1.001)
    {
        float3 V = normalize(g_CameraPos.xyz - s.WorldPos);
        float fresnel = 0.04 + 0.96 * pow(1.0 - saturate(dot(s.N, V)), 5.0);
        float3 tint = lerp(float3(1.0, 1.0, 1.0), s.Albedo, alpha);

        // The view through the glass replaces what is behind it, so the glass covers the pixel completely
        color += (1.0 - alpha) * (1.0 - fresnel) * tint * TraceRefraction(s, ior, RayNoise(pixelPos));
        coverage = 1.0;
    }
#endif

    return float4(ApplyFog(color, s.WorldPos, pixelPos), coverage);
}

#endif // HYBRID_SHADING_HLSLI
