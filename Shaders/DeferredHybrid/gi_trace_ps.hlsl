// gi_trace_ps.hlsl
// Per-pixel diffuse GI: traces this frame's rays (DiffuseGIPass). Paired with Deferred/fullscreen_vs.hlsl.
//
// For each GI pixel, a ray leaves the surface in the G-buffer in a random direction, more often near the normal
// (cosine-weighted, which is how a matte surface collects light). Whatever the ray finds is the light arriving from
// that direction: a surface, lit by the scene's lights, its own glow and the light it received itself last frame,
// or the sky. A ray or two per pixel is only a noisy sample of all the light arriving; SignalDenoiser then averages
// the rays over time and over neighbours.
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

// A single very bright sample (a ray that happens to land on a small strong light) would take many frames to average
// out and would show as a slowly fading blotch. Capping it trades a little lost light for a steady image.
// Where the object that was hit has an Emissive Light above 1, the limit grows with it (see TraceGIRay).
static const float GI_MAX_SAMPLE_LUMINANCE = 64.0;

// One ray from a surface point. Returns the radiance it found (rgb) and how open the surroundings are along it (a).
float4 TraceGIRay(float3 origin, float3 N, float3 geoN, float2 noise)
{
    // Cosine-weighted direction around the shading normal: a point on a disk, lifted onto the hemisphere
    float3 t, b;
    BuildBasis(N, t, b);
    float2 d = VogelDisk(0, 1, noise.x * 2.0 * PI, noise.y);
    // Normal maps can tilt the direction below the real surface; it is mirrored back above
    float3 dir = KeepAboveSurface(normalize(CosineHemisphereDirection(N, t, b, d)), geoN);

    float3 light = float3(0.0, 0.0, 0.0);
    float openness = 1.0;
    float maxLuminance = GI_MAX_SAMPLE_LUMINANCE;
    SceneHit hit;
    if (TraceSceneRay(origin, dir, g_GIMaxDistance, hit))
    {
        // A surface close by closes the surroundings off, like ambient occlusion
        openness = saturate(hit.Distance / AO_RADIUS);

        if (hit.Shadeable)
        {
            // Bounce light is soft, so the hit is shaded matte and its textures are read blurry (a wide footprint):
            // fine texture detail would only add noise. Its glow counts as the light it casts: its object's Emissive Light applies.
            uint hitFlags = HIT_EMISSIVE_LIGHT;
            if ((g_GIFlags & GI_FLAG_HIT_SHADOWS) != 0u) hitFlags |= HIT_SHADOWS;

            // Indirect light at the hit. If the hit was on screen last frame, the GI gathered there is exactly that:
            // adding it lets light bounce again and again, one more bounce per frame, for the cost of a texture read.
            // Elsewhere only the sky can be added, by checking with one more ray that it reaches the hit.
            float3 hitIndirect = float3(0.0, 0.0, 0.0);
            bool hasHitIndirect = false;
            if ((g_GIFlags & GI_FLAG_MORE_BOUNCES) != 0u)
            {
                BilinearTaps last = GIFindLastFrame(hit.Position, hit.GeoN);
                if (last.WeightSum > 0.01)
                {
                    hitIndirect = SanitizeLight(GILoadLastFrame(last).rgb);
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

            // A glow made brighter on purpose is a brighter sample on purpose: the limit rises with it, or the multiplier
            // would stop adding once the glow reached the limit
            maxLuminance *= max(hit.Material.EmissiveLight, 1.0);
        }
    }
    else
    {
        // The sky, from a blurred mip so a small bright sun is spread over many rays instead of found by a few
        light = SampleEnvSpecular(dir, 0.5) * g_SkyLightIntensity;
    }

    light = SanitizeLight(light);
    float luminance = Luminance(light);
    if (luminance > maxLuminance)
    {
        light *= maxLuminance / luminance;
    }
    return float4(light, openness);
}

// rgb: radiance this pixel's rays brought back, a: how open the surroundings are along them
float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    int2 giPixel = int2(In.Pos.xy);
    int2 viewPixel = GIViewPixel(giPixel);

    // Sky: nothing to light. The guide marks these pixels, so the later passes leave them out.
    float depth = g_GBufferDepth.Load(int3(viewPixel, 0)).r;
    if (depth >= 1.0)
    {
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    // The rays below are averaged over frames, so effects that need that (soft shadows at the hits) may rely on it
    s_RaysDenoised = (g_GIFlags & GI_FLAG_DENOISED) != 0u;

    // World position from depth, and how far off it can be (see DepthPositionError)
    float2 uv = (float2(viewPixel) + 0.5) * g_GIViewportSize.zw;
    float4 clipPos = UVToClip(uv, depth);
    float3 worldPos = ClipToWorld(clipPos, g_GIInvViewProj);
    float positionError = DepthPositionError(clipPos, worldPos, g_GIInvViewProj);

    float4 normals = g_GBufferNormals.Load(int3(viewPixel, 0));
    float3 N = OctDecode(normals.xy);
    float3 geoN = OctDecode(normals.zw);

    float3 origin = worldPos + geoN * (g_ShadowBias + positionError);

    // Unlike RayNoise, the pixel's random numbers don't wait for TAA: g_GIFrame moves them (see HashNoise)
    uint rayCount = max(g_GIRayCount, 1u);
    float4 sum = float4(0.0, 0.0, 0.0, 0.0);
    for (uint r = 0u; r < rayCount; ++r)
    {
        sum += TraceGIRay(origin, N, geoN, HashNoise(giPixel, g_GIFrame * rayCount + r));
    }
    return sum / float(rayCount);
}
