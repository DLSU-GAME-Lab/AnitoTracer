// Screen-space god rays (light shafts) from the main directional light. Paired with Deferred/fullscreen_vs.hlsl.
// Each pixel marches toward the sun's screen position, accumulating bright pixels (the sky around the sun)
// with exponential decay. Geometry in front of the sun is dark from the camera's side, so it naturally
// blocks the shafts without needing the depth buffer.
// Reads and writes the HDR scene color, before the display pass.

#include "hdr_common.hlsli"

Texture2D g_SceneColor;
SamplerState g_SceneColor_sampler;

// Mirrors Diligent::GodRaysConstants in src/Rendering/Pipelines/PipelineDefs.hpp
cbuffer GodRaysConstants
{
    float4 g_LightColor;    // rgb: light color * intensity, a: on-screen fade (0 when the sun is behind the camera)
    float2 g_SunUV;         // Sun position in UV space; may lie outside 0..1
    float g_Density;        // Fraction of the distance to the sun covered by the march
    float g_Decay;          // Per-sample falloff
    float g_Weight;         // Per-sample contribution
    float g_Threshold;      // Luminance below which pixels don't emit shafts (HDR, so it can be above 1)
    float g_SunRadius;      // Radius (in UV, aspect-corrected) of the emitting region around the sun
    float g_AspectRatio;    // Width / height
    int g_SampleCount;
    float g_Knee;           // Luminance range above the threshold over which pixels fade in to full emission
    float g_MaxBrightness;  // Luminance cap of an emitting pixel
    float g_Padding;
};

struct FullScreenPSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

static const float3 LUMA_WEIGHTS = float3(0.2126, 0.7152, 0.0722);

// How much a sample at this UV emits: bright pixels close to the sun
float3 SampleOccluder(float2 uv)
{
    float3 color = SanitizeHDR(g_SceneColor.SampleLevel(g_SceneColor_sampler, uv, 0).rgb);
    float luma = dot(color, LUMA_WEIGHTS);
    float bright = saturate((luma - g_Threshold) / g_Knee);

    // Scene color has no upper limit: without a cap, a few very bright pixels (the sun's disk, a strong emissive)
    // would outweigh everything else along the march and flicker as they move
    color *= min(1.0, g_MaxBrightness / max(luma, 1e-4));

    float2 toSun = (uv - g_SunUV) * float2(g_AspectRatio, 1.0);
    float sunMask = 1.0 - smoothstep(0.0, g_SunRadius, length(toSun));

    return color * bright * sunMask;
}

// Interleaved gradient noise, used to jitter the march start and trade banding for fine grain
float InterleavedGradientNoise(float2 pixel)
{
    return frac(52.9829189 * frac(dot(pixel, float2(0.06711056, 0.00583715))));
}

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    float4 scene = g_SceneColor.Sample(g_SceneColor_sampler, In.UV);

    int sampleCount = clamp(g_SampleCount, 1, 128);
    float2 delta = (In.UV - g_SunUV) * (g_Density / sampleCount);
    float2 uv = In.UV - delta * InterleavedGradientNoise(In.Pos.xy);

    float3 shafts = 0.0;
    float illumination = 1.0;
    for (int i = 0; i < sampleCount; ++i)
    {
        uv -= delta;
        shafts += SampleOccluder(uv) * illumination * g_Weight;
        illumination *= g_Decay;
    }

    shafts *= g_LightColor.rgb * g_LightColor.a;

    return float4(scene.rgb + shafts, scene.a);
}
