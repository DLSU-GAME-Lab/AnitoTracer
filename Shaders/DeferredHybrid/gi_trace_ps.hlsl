// gi_trace_ps.hlsl
// Per-pixel diffuse GI, pass 1 of 3 (DiffuseGIPass): traces this frame's rays. Paired with Deferred/fullscreen_vs.hlsl.
//
// For each GI pixel, a ray leaves the surface in the G-buffer in a random direction, more often near the normal
// (cosine-weighted, which is how a matte surface collects light). Whatever the ray finds is the light arriving from
// that direction: a surface, lit by the scene's lights, its own glow and the light it received itself last frame,
// or the sky. A ray or two per pixel is only a noisy sample of all the light arriving; the next two passes average
// rays over time and over neighbours.
//
// Only built where ray hits can be shaded (RT_SCENE_SHADING, see rt_scene.hlsli).
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../fog.hlsli"
#include "../hybrid_shading.hlsli"
#include "gbuffer_common.hlsli"
#include "gi_common.hlsli"

Texture2D g_GBufferNormals;
Texture2D g_GBufferDepth;

struct GITraceOutput
{
    float4 Light : SV_TARGET0; // rgb: radiance this pixel's rays brought back, a: how open the surroundings are along them
    float4 Guide : SV_TARGET1; // xy: octahedral geometric normal, z: distance from the camera, w: 1 where there is a surface
};

// A single very bright sample (a ray that happens to land on a small strong light) would take many frames to average
// out and would show as a slowly fading blotch. Capping it trades a little lost light for a steady image.
static const float GI_MAX_SAMPLE_LUMINANCE = 64.0;

// Scrambles an integer into another with no visible pattern (PCG hash)
uint GIHash(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// Two random numbers in [0, 1) for the index-th ray this pixel has ever traced. Unlike RayNoise this does not wait
// for TAA: the GI averages over frames on its own.
// Each pixel starts at its own random point and then steps through a sequence that fills the unit square evenly
// (the R2 sequence), so the directions a pixel tries over a few dozen rays cover the hemisphere without clumps or
// gaps, and neighbouring pixels don't try the same ones.
float2 GINoise(int2 giPixel, uint index)
{
    uint h1 = GIHash(uint(giPixel.x) + GIHash(uint(giPixel.y)));
    uint h2 = GIHash(h1);
    float2 start = float2(h1 & 0xFFFFu, h2 & 0xFFFFu) / 65536.0;
    return frac(start + float(index % 4096u) * float2(0.7548776662, 0.5698402910));
}

// One ray from a surface point. Returns the radiance it found (rgb) and how open the surroundings are along it (a).
float4 TraceGIRay(float3 origin, float3 N, float3 geoN, float2 noise)
{
    // Cosine-weighted direction around the shading normal: a point on a disk, lifted onto the hemisphere
    float3 t, b;
    BuildBasis(N, t, b);
    float2 d = VogelDisk(0, 1, noise.x * 2.0 * PI, noise.y);
    float3 dir = normalize(t * d.x + b * d.y + N * sqrt(saturate(1.0 - dot(d, d))));

    // Normal maps can tilt the direction below the real surface; mirror it back above
    float below = dot(dir, geoN);
    if (below < 0.0)
    {
        dir -= 2.0 * below * geoN;
    }

    float3 light = float3(0.0, 0.0, 0.0);
    float openness = 1.0;
    SceneHit hit;
    if (TraceSceneRay(origin, dir, g_GIMaxDistance, hit))
    {
        // A surface close by closes the surroundings off, like ambient occlusion
        openness = saturate(hit.Distance / AO_RADIUS);

        if (hit.Shadeable)
        {
            // Bounce light is soft, so the hit is shaded matte and its textures are read blurry (a wide footprint):
            // fine texture detail would only add noise
            uint hitFlags = 0u;
            if ((g_GIFlags & GI_FLAG_HIT_SHADOWS) != 0u) hitFlags |= HIT_SHADOWS;

            // Indirect light at the hit. If the hit was on screen last frame, the GI gathered there is exactly that:
            // adding it lets light bounce again and again, one more bounce per frame, for the cost of a texture read.
            // Elsewhere only the sky can be added, by checking with one more ray that it reaches the hit.
            float3 hitIndirect = float3(0.0, 0.0, 0.0);
            bool hasHitIndirect = false;
            if ((g_GIFlags & GI_FLAG_MORE_BOUNCES) != 0u)
            {
                GILastFrame last = GIFindLastFrame(hit.Position, hit.GeoN);
                if (last.WeightSum > 0.01)
                {
                    hitIndirect = GISanitize(GILoadLastFrame(last).rgb);
                    hasHitIndirect = true;
                }
            }
            if (hasHitIndirect)
            {
                hitFlags |= HIT_AMBIENT_GIVEN;
            }
            else if ((g_GIFlags & GI_FLAG_SKY_BOUNCE) != 0u)
            {
                hitFlags |= HIT_AMBIENT_TRACED;
            }

            light = ShadeSceneHit(hit, dir, 0.1 * hit.Distance, ShiftNoise(noise, 1), hitFlags, hitIndirect);
        }
    }
    else
    {
        // The sky, from a blurred mip so a small bright sun is spread over many rays instead of found by a few
        light = SampleEnvSpecular(dir, 0.5) * g_SkyLightIntensity;
    }

    light = GISanitize(light);
    float luminance = dot(light, float3(0.2126, 0.7152, 0.0722));
    if (luminance > GI_MAX_SAMPLE_LUMINANCE)
    {
        light *= GI_MAX_SAMPLE_LUMINANCE / luminance;
    }
    return float4(light, openness);
}

void main_ps(in FullScreenPSInput In, out GITraceOutput Out)
{
    int2 giPixel = int2(In.Pos.xy);
    int2 viewPixel = GIViewPixel(giPixel);

    Out.Light = float4(0.0, 0.0, 0.0, 1.0);
    Out.Guide = float4(0.0, 0.0, 0.0, 0.0);

    // Sky: nothing to light. The guide's w = 0 tells the later passes to leave this pixel out.
    float depth = g_GBufferDepth.Load(int3(viewPixel, 0)).r;
    if (depth >= 1.0)
    {
        return;
    }

    // World position from depth, and how far off it can be (see lighting_ps.hlsl)
    float2 uv = (float2(viewPixel) + 0.5) * g_GIViewportSize.zw;
    float4 clipPos = float4(uv.x * 2.0 - 1.0, 1.0 - 2.0 * uv.y, depth, 1.0);
    float4 worldPos = mul(clipPos, g_GIInvViewProj);
    worldPos.xyz /= worldPos.w;

    float depthStep = asfloat(asuint(depth) + 2) - depth;
    float4 worldPosNext = mul(float4(clipPos.xy, depth + depthStep, 1.0), g_GIInvViewProj);
    float positionError = length(worldPosNext.xyz / worldPosNext.w - worldPos.xyz);

    float4 normals = g_GBufferNormals.Load(int3(viewPixel, 0));
    float3 N = OctDecode(normals.xy);
    float3 geoN = OctDecode(normals.zw);

    // The guide uses the geometric normal: a normal map's fine detail would make neighbouring pixels look like
    // different surfaces and stop the blur from averaging them
    float viewDistance = length(worldPos.xyz - g_GICameraPos.xyz);
    Out.Guide = float4(normals.zw, viewDistance, 1.0);

    float3 origin = worldPos.xyz + geoN * (g_ShadowBias + positionError);

    uint rayCount = max(g_GIRayCount, 1u);
    float4 sum = float4(0.0, 0.0, 0.0, 0.0);
    for (uint r = 0u; r < rayCount; ++r)
    {
        sum += TraceGIRay(origin, N, geoN, GINoise(giPixel, g_GIFrame * rayCount + r));
    }
    Out.Light = sum / float(rayCount);
}
