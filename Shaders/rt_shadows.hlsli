#ifndef RT_SHADOWS_HLSLI
#define RT_SHADOWS_HLSLI

// Ray-traced visibility against the scene TLAS, shared by every pipeline that traces shadow rays
// (hybrid_shading.hlsli, CrossHatch/crosshatch_ps.hlsl).
// Requires light_struct.hlsli (g_ShadowBias, g_NoiseFrame) to be included first.

#include "Common/math_common.hlsli"
#include "Common/sampling_common.hlsli"

// Master, we bind our Scene Acceleration Structure for Ray Queries here!
RaytracingAccelerationStructure g_TLAS;

// Bits of g_TransparencyFlags, from the project's transparency settings (mirror TransparencyFlags in PipelineDefs.hpp)
static const uint TRANSPARENCY_TEST_RAYS = 1u;          // Rays test cutouts and glass; without it every triangle is solid
static const uint TRANSPARENCY_CUTOUT = 2u;             // A cutout blocks rays only where its alpha reaches the cutoff
static const uint TRANSPARENCY_GLASS = 4u;              // Glass counts: it is drawn, and rays see it. Without it rays go straight through.
static const uint TRANSPARENCY_TINTED_SHADOWS = 8u;     // Light rays cross glass and pick up its color; without it glass blocks them
static const uint TRANSPARENCY_REFRACTION = 16u;        // Glass bends what is behind it

// Cutouts and glass can only be told apart from solid walls by reading the scene's tables, which needs the same
// device support as shading what a ray hits. Without it every triangle is solid.
//
// The code that judges them (RT_ALPHA_TEST) slows every ray down a little even where nothing needs judging, so
// the pipelines only compile it in while the scene holds a cutout or a pane of glass. Without it every triangle
// is solid, and rays take the fastest path there is.
#ifdef RT_SCENE_SHADING
#include "rt_scene_data.hlsli"
#endif

// ------------------------------------------------------------------
// Ray budget / quality knobs
// ------------------------------------------------------------------
static const int SHADOW_SAMPLES = 4;            // Rays per light, spread over the light's area, where nothing cleans up single rays
// The size of a light, and with it how soft its shadows are, belongs to the light: Direction.w of a directional light
// (its angular radius), Position.w of a point or spot light (the radius of its body)

// ------------------------------------------------------------------
// Sampling helpers
// ------------------------------------------------------------------

// Rotation (x) and radial offset (y) for one pixel's sample pattern, both in [0, 1).
// While TAA accumulates frames (g_NoiseFrame > 0) the pattern changes every frame, so the
// accumulated result covers many more samples than are traced per frame. Without TAA it stays
// still (radial offset 0.5, the evenly spread pattern), since a changing pattern would only flicker.
float2 RayNoise(float2 pixelPos)
{
    if (g_NoiseFrame == 0)
    {
        return float2(InterleavedGradientNoise(pixelPos), 0.5);
    }
    float2 frameOffset = 5.588238 * float(g_NoiseFrame % 64);
    return float2(InterleavedGradientNoise(pixelPos + frameOffset),
                  InterleavedGradientNoise(pixelPos + float2(47.0, 17.0) + frameOffset));
}

// Same noise, decorrelated for the k-th light or effect so their patterns don't line up (golden ratio shift)
float2 ShiftNoise(float2 noise, int k)
{
    return float2(frac(noise.x + 0.618034 * k), frac(noise.y + 0.754878 * k));
}

// Two random numbers in [0, 1) for the index-th ray a pixel has traced, for passes whose rays are averaged over
// frames by a denoiser. Unlike RayNoise it does not wait for TAA: pass a counter that goes up every frame.
// Each pixel starts at its own random point and then steps through a sequence that fills the unit square evenly
// (the R2 sequence), so the rays a pixel traces over a few dozen frames cover their range without clumps or gaps,
// and neighbouring pixels don't try the same ones.
float2 HashNoise(int2 pixel, uint index)
{
    uint h1 = HashUint(uint(pixel.x) + HashUint(uint(pixel.y)));
    uint h2 = HashUint(h1);
    float2 start = float2(h1 & 0xFFFFu, h2 & 0xFFFFu) / 65536.0;
    return frac(start + float(index % 4096u) * float2(0.7548776662, 0.5698402910));
}

// Set by a pass whose rays a denoiser averages over frames (SignalDenoiser), which happens with TAA off too
static bool s_RaysDenoised = false;

// True while something averages each pixel's rays over several frames: TAA, or a denoiser. Effects that only work
// as an average of many rays (a soft edge from one ray per frame, a blurred reflection) check this, and fall back
// to their sharp, noise-free version otherwise.
bool RaysAveraged()
{
    return g_NoiseFrame != 0u || s_RaysDenoised;
}

// ------------------------------------------------------------------
// Helper: Traces a visibility ray, returns 1 when nothing is hit
// positionError: how far worldPos may be off the real surface (deferred depth reconstruction), added to the bias
// lightRay: the ray looks for what blocks light (a shadow ray). Occlusion rays (false) go through glass.
// transmittance: the color the glass in the way lets through, 1 without any. Only meaningful when the result is 1.
// ------------------------------------------------------------------
float TraceVisibilityRay(float3 worldPos, float3 normal, float positionError, float3 lightDir, float maxDist, bool lightRay, out float3 transmittance)
{
    // Master! We offset the ray origin along the unperturbed geometric normal to prevent self-shadowing~♡
    RayDesc ray;
    ray.Origin = worldPos + (normal * (g_ShadowBias + positionError));
    ray.Direction = lightDir;
    ray.TMin = 0.001;
    ray.TMax = maxDist;

    transmittance = float3(1.0, 1.0, 1.0);

    RayQuery < RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER > q;

#if defined(RT_SCENE_SHADING) && defined(RT_ALPHA_TEST)
    if ((g_TransparencyFlags & TRANSPARENCY_TEST_RAYS) != 0u)
    {
        // Triangles flagged opaque end the search by themselves. The rest (cutouts and glass) come up here to be judged.
        q.TraceRayInline(g_TLAS, RAY_FLAG_NONE, 0xFF, ray);

        while (q.Proceed())
        {
            if (q.CandidateType() != CANDIDATE_NON_OPAQUE_TRIANGLE)
            {
                continue;
            }

            SceneGeometryRecord candidate = g_SceneGeometry[q.CandidateInstanceID() + q.CandidateGeometryIndex()];
            uint primitive = q.CandidatePrimitiveIndex();
            float2 bary = q.CandidateTriangleBarycentrics();

            if ((candidate.Flags & SCENE_GEOMETRY_BLEND) != 0u)
            {
                // Glass that isn't there, or occlusion rays, which pass through it (a window doesn't darken a room's corners)
                if ((g_TransparencyFlags & TRANSPARENCY_GLASS) == 0u || !lightRay)
                {
                    continue;
                }
                if ((g_TransparencyFlags & TRANSPARENCY_TINTED_SHADOWS) == 0u)
                {
                    q.CommitNonOpaqueTriangleHit();
                    continue;
                }
                // A blocker found beyond this pane makes the result 0 anyway, so the order they are met in doesn't matter
                transmittance *= GlassTransmittanceAt(candidate, primitive, bary);
                continue;
            }

            if (SceneCutoutBlocks(candidate, primitive, bary))
            {
                q.CommitNonOpaqueTriangleHit();
            }
        }

        return q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0.0 : 1.0;
    }
#endif

    // Force opaque so the RayQuery doesn't wait for any-hit shader confirmation!
    q.TraceRayInline(g_TLAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);

    // Traversal state machine loop~
    while (q.Proceed())
    {
    }

    return q.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? 0.0 : 1.0;
}

// A shadow ray: 1 when the light gets through, with the color glass tints it in `transmittance`
float TraceShadowRay(float3 worldPos, float3 normal, float positionError, float3 lightDir, float maxDist, out float3 transmittance)
{
    return TraceVisibilityRay(worldPos, normal, positionError, lightDir, maxDist, true, transmittance);
}

float TraceShadowRay(float3 worldPos, float3 normal, float positionError, float3 lightDir, float maxDist)
{
    float3 transmittance;
    return TraceVisibilityRay(worldPos, normal, positionError, lightDir, maxDist, true, transmittance);
}

// An ambient occlusion ray: 1 when nothing is in the way. Glass doesn't count.
float TraceOcclusionRay(float3 worldPos, float3 normal, float positionError, float3 dir, float maxDist)
{
    float3 transmittance;
    return TraceVisibilityRay(worldPos, normal, positionError, dir, maxDist, false, transmittance);
}

// Where the shadow rays of the soft shadow functions below go. A directional light is a disk in the sky around L
// (spread: tangent of its angular radius), a point or spot light a disk of its radius around its position, facing the
// surface. t and b are two directions at right angles to L, see BuildBasis; noise and samples as for the shadow functions.
float3 DirectionalShadowRayDirection(float3 L, float3 t, float3 b, float spread, float2 noise, int i, int samples)
{
    float2 d = VogelDisk(i, samples, noise.x * 2.0 * PI, noise.y) * spread;
    return normalize(L + t * d.x + b * d.y);
}

float3 PointShadowRayTarget(float3 lightPos, float3 t, float3 b, float radius, float2 noise, int i, int samples)
{
    float2 d = VogelDisk(i, samples, noise.x * 2.0 * PI, noise.y) * radius;
    return lightPos + t * d.x + b * d.y;
}

// Same as TraceDirectionalShadow, but per colour channel: the share of each that reaches the surface. They differ
// where glass tints the light on its way; without any all three are the scalar result.
// spread: tangent of the angular radius of the light's disk
float3 TraceDirectionalShadowRGB(float3 worldPos, float3 geoN, float positionError, float3 L, float spread, float2 noise, int samples)
{
    float3 t, b;
    BuildBasis(L, t, b);

    float3 visibility = float3(0.0, 0.0, 0.0);
    for (int s = 0; s < samples; ++s)
    {
        float3 transmittance;
        float v = TraceShadowRay(worldPos, geoN, positionError, DirectionalShadowRayDirection(L, t, b, spread, noise, s, samples), 1000.0, transmittance);
        visibility += v * transmittance;
    }
    return visibility / float(samples);
}

// Same for a point or spot light: a disk of the light's radius that faces the surface
float3 TracePointShadowRGB(float3 worldPos, float3 geoN, float positionError, float3 lightPos, float radius, float3 L, float2 noise, int samples)
{
    float3 t, b;
    BuildBasis(L, t, b);

    float3 visibility = float3(0.0, 0.0, 0.0);
    for (int s = 0; s < samples; ++s)
    {
        float3 toSample = PointShadowRayTarget(lightPos, t, b, radius, noise, s, samples) - worldPos;
        float dist = length(toSample);
        float3 transmittance;
        float v = TraceShadowRay(worldPos, geoN, positionError, toSample / dist, dist - 0.01, transmittance);
        visibility += v * transmittance;
    }
    return visibility / float(samples);
}

// Same for a rectangular light: the rays go to points spread over the panel (a low-discrepancy pattern that the pixel's
// noise shifts, so neighbouring pixels sample different points). axisX and axisY are the half width and half height
// as world vectors, see PointLight in light_struct.hlsli. A point of the panel below the surface's horizon counts as
// lit: that part of the panel adds no light there anyway.
float3 TraceRectShadowRGB(float3 worldPos, float3 geoN, float positionError, float3 center, float3 axisX, float3 axisY, float2 noise, int samples)
{
    float3 visibility = float3(0.0, 0.0, 0.0);
    for (int s = 0; s < samples; ++s)
    {
        float2 uv = frac(noise + (float(s) + 0.5) * float2(0.7548776662, 0.5698402910)) * 2.0 - 1.0;
        float3 toSample = center + axisX * uv.x + axisY * uv.y - worldPos;
        float dist = length(toSample);
        float3 dir = toSample / max(dist, 0.0001);
        if (dot(dir, geoN) <= 0.0)
        {
            visibility += float3(1.0, 1.0, 1.0);
            continue;
        }
        float3 transmittance;
        float v = TraceShadowRay(worldPos, geoN, positionError, dir, dist - 0.01, transmittance);
        visibility += v * transmittance;
    }
    return visibility / float(samples);
}

// Soft shadow from a directional light treated as a disk, from `samples` rays spread over it. spread is the tangent of
// the disk's angular radius.
// noise comes from RayNoise or HashNoise. One ray gives 0 or 1: a soft edge then only appears once frames are averaged.
float TraceDirectionalShadow(float3 worldPos, float3 geoN, float positionError, float3 L, float spread, float2 noise, int samples)
{
    float3 t, b;
    BuildBasis(L, t, b);

    float visibility = 0.0;
    for (int s = 0; s < samples; ++s)
    {
        visibility += TraceShadowRay(worldPos, geoN, positionError, DirectionalShadowRayDirection(L, t, b, spread, noise, s, samples), 1000.0);
    }
    return visibility / float(samples);
}

float TraceDirectionalShadow(float3 worldPos, float3 geoN, float positionError, float3 L, float spread, float2 noise)
{
    return TraceDirectionalShadow(worldPos, geoN, positionError, L, spread, noise, SHADOW_SAMPLES);
}

// Soft shadow from a point light treated as a disk of the light's radius facing the surface, from `samples` rays
float TracePointShadow(float3 worldPos, float3 geoN, float positionError, float3 lightPos, float radius, float3 L, float2 noise, int samples)
{
    float3 t, b;
    BuildBasis(L, t, b);

    float visibility = 0.0;
    for (int s = 0; s < samples; ++s)
    {
        float3 toSample = PointShadowRayTarget(lightPos, t, b, radius, noise, s, samples) - worldPos;
        float dist = length(toSample);
        visibility += TraceShadowRay(worldPos, geoN, positionError, toSample / dist, dist - 0.01);
    }
    return visibility / float(samples);
}

float TracePointShadow(float3 worldPos, float3 geoN, float positionError, float3 lightPos, float radius, float3 L, float2 noise)
{
    return TracePointShadow(worldPos, geoN, positionError, lightPos, radius, L, noise, SHADOW_SAMPLES);
}

#endif // RT_SHADOWS_HLSLI
