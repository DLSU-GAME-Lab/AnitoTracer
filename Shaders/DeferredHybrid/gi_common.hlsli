#ifndef DEFERRED_HYBRID_GI_COMMON_HLSLI
#define DEFERRED_HYBRID_GI_COMMON_HLSLI

// Constants and last-frame lookup of the per-pixel diffuse GI trace pass (DiffuseGIPass, gi_trace_ps.hlsl).
//
// The GI buffers are g_GIScale times smaller than the view along each axis (1 or 2). GI pixel p stands for the view
// pixel p * g_GIScale: that is where its ray starts and where its guide (normal, distance) is.
//
// Light is stored as the average radiance arriving over the hemisphere (irradiance / PI), the same unit
// SampleEnvIrradiance returns, so the lighting pass can use either one the same way.
//
// The rays are averaged over time and blurred afterwards by SignalDenoiser, like the other ray-traced signals.

#include "gbuffer_common.hlsli"
#include "temporal_common.hlsli"
#include "../Common/math_common.hlsli"
#include "../Common/screen_common.hlsli"

// Mirrors GIConstants in DiffuseGIPass.cpp
cbuffer GIConstants

{
    float4x4 g_GIInvViewProj;   // This frame's clip space -> world (the matrix the G-buffer was drawn with)
    float4x4 g_GIPrevViewProj;  // World -> last frame's clip space
    float4 g_GICameraPos;
    float4 g_GIPrevCameraPos;
    float4 g_GIViewportSize;    // xy: view size in pixels, zw: 1 / size
    float4 g_GISize;            // xy: GI buffer size in pixels, zw: 1 / size

    uint g_GIScale;             // View pixels per GI pixel along each axis
    uint g_GIFrame;             // Moves the rays: counts up every frame while they are averaged over time, else stays put
    float g_GIHistoryValid;     // 0: there is no last frame to read (further bounces fall back to the sky)
    float g_GIMaxDistance;      // How far a GI ray looks for surfaces

    uint g_GIFlags;             // GI_FLAG_* bits
    uint g_GIRayCount;          // Rays traced per GI pixel every frame
    float2 g_GIPadding;
};


static const uint GI_FLAG_HIT_SHADOWS = 1u;     // Trace shadow rays from what a GI ray hits
static const uint GI_FLAG_SKY_BOUNCE = 2u;      // What a GI ray hits is also lit by the sky, where one more ray reaches it
static const uint GI_FLAG_MORE_BOUNCES = 4u;    // What a GI ray hits is also lit by last frame's GI, so light keeps bouncing
static const uint GI_FLAG_DENOISED = 8u;        // A denoiser averages the rays over frames (so they may vary from frame to frame)

// Last frame's GI as it was shown (GI sized), and last frame's guide (view sized, see deferred_common.hlsli)
Texture2D g_GIPrevResult;
Texture2D g_GIPrevGuide;

// The view pixel a GI pixel stands for
int2 GIViewPixel(int2 giPixel)
{
    return SignalToViewPixel(giPixel, int(g_GIScale), int2(g_GIViewportSize.xy));
}

// Where a world position was in last frame's GI: the four GI pixels around it and how much each counts.
// A pixel only counts if last frame it showed the same surface (its guide has a matching distance and normal), so
// WeightSum is 0 when the position was hidden or off screen then.
BilinearTaps GIFindLastFrame(float3 worldPos, float3 N)
{
    BilinearTaps last = NoTaps();

    float4 prevClip = mul(float4(worldPos, 1.0), g_GIPrevViewProj);
    if (g_GIHistoryValid < 0.5 || prevClip.w <= 0.0)
    {
        return last;
    }

    float2 prevUV = ClipToUV(prevClip);

    // Position among last frame's GI pixels, whose data sits at the centre of view pixel p * g_GIScale
    float2 samplePos = (prevUV * g_GIViewportSize.xy - 0.5) / float(g_GIScale);
    last.Base = int2(floor(samplePos));
    float2 f = samplePos - float2(last.Base);

    float expectedDistance = length(worldPos - g_GIPrevCameraPos.xyz);
    int2 maxPixel = int2(g_GISize.xy) - 1;

    for (int k = 0; k < 4; ++k)
    {
        int2 offset = BilinearTapOffset(k);
        int2 tap = last.Base + offset;
        if (any(tap < int2(0, 0)) || any(tap > maxPixel))
            continue;

        float4 prevGuide = g_GIPrevGuide.Load(int3(GIViewPixel(tap), 0));
        if (prevGuide.w <= 0.0)
            continue;
        if (abs(prevGuide.z - expectedDistance) > 0.05 * expectedDistance + 0.02)
            continue;
        if (dot(OctDecode(prevGuide.xy), N) < 0.9)
            continue;

        float w = BilinearTapWeight(offset, f);
        last.Weights[k] = w;
        last.WeightSum += w;
    }
    return last;
}

// Last frame's GI at a position found with GIFindLastFrame (only meaningful when WeightSum > 0)
float4 GILoadLastFrame(BilinearTaps last)
{
    return LoadBilinearTaps(g_GIPrevResult, last);
}


#endif // DEFERRED_HYBRID_GI_COMMON_HLSLI
