#ifndef RT_SHADOWS_HLSLI
#define RT_SHADOWS_HLSLI

// Ray-traced visibility against the scene TLAS, shared by every pipeline that traces shadow rays
// (hybrid_shading.hlsli, CrossHatch/crosshatch_ps.hlsl).
// Requires light_struct.hlsli (g_ShadowBias, g_NoiseFrame), pbr_defs.hlsli (PI) and fog.hlsli (InterleavedGradientNoise) to be included first.

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
static const float SUN_ANGULAR_RADIUS = 0.01;   // Radians. Real sun is ~0.0047, slightly larger reads better
static const float POINT_LIGHT_RADIUS = 0.1;    // World units, gives point lights a soft penumbra

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

// Scrambles an integer into another with no visible pattern (PCG hash)
uint HashUint(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
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

// Point i of n on a unit disk, evenly spread, rotated by phi (radians).
// radialOffset in [0, 1) places the sample within its ring; 0.5 is the ring's middle.
float2 VogelDisk(int i, int n, float phi, float radialOffset)
{
    float r = sqrt((i + radialOffset) / n);
    float theta = i * 2.39996323 + phi; // Golden angle
    return r * float2(cos(theta), sin(theta));
}

void BuildBasis(float3 n, out float3 t, out float3 b)
{
    float3 up = abs(n.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    t = normalize(cross(up, n));
    b = cross(n, t);
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

// Rec. 709 luminance weights: how much each colour channel adds to brightness
static const float3 LUMINANCE_WEIGHTS = float3(0.2126, 0.7152, 0.0722);

// Same as TraceDirectionalShadow, but per colour channel: the share of each that reaches the surface. They differ
// where glass tints the light on its way; without any all three are the scalar result.
float3 TraceDirectionalShadowRGB(float3 worldPos, float3 geoN, float positionError, float3 L, float2 noise, int samples)
{
    float3 t, b;
    BuildBasis(L, t, b);
    float spread = tan(SUN_ANGULAR_RADIUS);

    float3 visibility = float3(0.0, 0.0, 0.0);
    for (int s = 0; s < samples; ++s)
    {
        float2 d = VogelDisk(s, samples, noise.x * 2.0 * PI, noise.y) * spread;
        float3 transmittance;
        float v = TraceShadowRay(worldPos, geoN, positionError, normalize(L + t * d.x + b * d.y), 1000.0, transmittance);
        visibility += v * transmittance;
    }
    return visibility / float(samples);
}

// Same for a point light
float3 TracePointShadowRGB(float3 worldPos, float3 geoN, float positionError, float3 lightPos, float3 L, float2 noise, int samples)
{
    float3 t, b;
    BuildBasis(L, t, b);

    float3 visibility = float3(0.0, 0.0, 0.0);
    for (int s = 0; s < samples; ++s)
    {
        float2 d = VogelDisk(s, samples, noise.x * 2.0 * PI, noise.y) * POINT_LIGHT_RADIUS;
        float3 toSample = lightPos + t * d.x + b * d.y - worldPos;
        float dist = length(toSample);
        float3 transmittance;
        float v = TraceShadowRay(worldPos, geoN, positionError, toSample / dist, dist - 0.01, transmittance);
        visibility += v * transmittance;
    }
    return visibility / float(samples);
}

// Soft shadow from a directional light treated as a disk of SUN_ANGULAR_RADIUS, from `samples` rays spread over it.
// noise comes from RayNoise or HashNoise. One ray gives 0 or 1: a soft edge then only appears once frames are averaged.
float TraceDirectionalShadow(float3 worldPos, float3 geoN, float positionError, float3 L, float2 noise, int samples)
{
    float3 t, b;
    BuildBasis(L, t, b);
    float spread = tan(SUN_ANGULAR_RADIUS);

    float visibility = 0.0;
    for (int s = 0; s < samples; ++s)
    {
        float2 d = VogelDisk(s, samples, noise.x * 2.0 * PI, noise.y) * spread;
        visibility += TraceShadowRay(worldPos, geoN, positionError, normalize(L + t * d.x + b * d.y), 1000.0);
    }
    return visibility / float(samples);
}

float TraceDirectionalShadow(float3 worldPos, float3 geoN, float positionError, float3 L, float2 noise)
{
    return TraceDirectionalShadow(worldPos, geoN, positionError, L, noise, SHADOW_SAMPLES);
}

// Soft shadow from a point light treated as a disk of POINT_LIGHT_RADIUS facing the surface, from `samples` rays
float TracePointShadow(float3 worldPos, float3 geoN, float positionError, float3 lightPos, float3 L, float2 noise, int samples)
{
    float3 t, b;
    BuildBasis(L, t, b);

    float visibility = 0.0;
    for (int s = 0; s < samples; ++s)
    {
        float2 d = VogelDisk(s, samples, noise.x * 2.0 * PI, noise.y) * POINT_LIGHT_RADIUS;
        float3 toSample = lightPos + t * d.x + b * d.y - worldPos;
        float dist = length(toSample);
        visibility += TraceShadowRay(worldPos, geoN, positionError, toSample / dist, dist - 0.01);
    }
    return visibility / float(samples);
}

float TracePointShadow(float3 worldPos, float3 geoN, float positionError, float3 lightPos, float3 L, float2 noise)
{
    return TracePointShadow(worldPos, geoN, positionError, lightPos, L, noise, SHADOW_SAMPLES);
}

#endif // RT_SHADOWS_HLSLI
