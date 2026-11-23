#ifndef DEFERRED_HYBRID_DENOISE_COMMON_HLSLI
#define DEFERRED_HYBRID_DENOISE_COMMON_HLSLI

// Shared by the two passes of SignalDenoiser: denoise_temporal_ps (average each pixel's rays over frames) and
// denoise_filter_ps (blur what noise is left, without crossing edges). Our own filters, in the style of SVGF
// (Schied et al. 2017, "Spatiotemporal Variance-Guided Filtering").
//
// A "signal" is what one kind of ray found per pixel (see deferred_common.hlsli). Its texture is g_DnScale times
// smaller than the view along each axis (1 or 2); signal pixel p stands for the view pixel p * g_DnScale, which is
// where its entry in the guide and in the motion vectors is.

#include "gbuffer_common.hlsli"

// Mirrors DenoiseConstants in SignalDenoiser.cpp
cbuffer DenoiseConstants
{
    float4x4 g_DnInvViewProj;           // This frame's clip space -> world (the matrix the G-buffer was drawn with)
    float4x4 g_DnViewProjNoJitter;      // World -> clip space this frame, without TAA jitter
    float4x4 g_DnPrevViewProjNoJitter;  // The same for last frame
    float4 g_DnCameraPos;
    float4 g_DnPrevCameraPos;
    float4 g_DnViewportSize;            // xy: view size in pixels, zw: 1 / size
    float4 g_DnSize;                    // xy: signal size in pixels, zw: 1 / size

    uint g_DnScale;                     // View pixels per signal pixel along each axis
    uint g_DnKind;                      // SIGNAL_* below
    float g_DnHistoryValid;             // 0: there is no last frame to blend with
    float g_DnMinBlend;                 // Smallest share the new frame gets

    float g_DnClampSigma;               // Last frame's value is pulled into this many deviations around this frame's
                                        // neighbourhood; 0 trusts it as it is
    float g_DnBrightnessSigma;          // The blur leaves out neighbours that differ by more than this many times
                                        // the signal's own uncertainty; 0 blurs whatever lies on the same surface
    int g_DnFilterStep;                 // Pixel spacing of the blur pass being drawn
    float g_DnUncertaintyScale;         // Shrinks the uncertainty for the later blur passes, which see a cleaner signal

    float g_DnPixelAngle;               // Angle one view pixel covers, in radians
    float g_DnMaxRoughness;             // Reflections: no ray was traced for surfaces at least this rough
    float g_DnVirtualMotion;            // Reflections: 1 looks last frame's value up where the reflected image was
    float g_DnPadding;
};

// What a signal holds, which decides how its noise is measured
static const uint SIGNAL_VISIBILITY = 0u;   // Four independent values in 0..1, one per channel (shadows of four lights)
static const uint SIGNAL_LIGHT = 1u;        // rgb light (or a direction), a: a fourth value blurred along with it
static const uint SIGNAL_REFLECTION = 2u;   // rgb light, a: distance to what is reflected, which sizes the blur

// xy: octahedral geometric normal, z: distance from the camera, w: roughness (0: sky). View sized.
Texture2D g_DnGuide;

struct FullScreenPSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

// The view pixel a signal pixel stands for
int2 DnViewPixel(int2 signalPixel)
{
    return min(signalPixel * int(g_DnScale), int2(g_DnViewportSize.xy) - 1);
}

float4 DnLoadGuide(Texture2D guide, int2 signalPixel)
{
    return guide.Load(int3(DnViewPixel(signalPixel), 0));
}

// From the camera through a position on screen, given in view pixels (a pixel's centre is at +0.5). Not normalized.
// It changes linearly across the screen, so a pixel's direction and its differences to the next pixel right and
// down give the direction of every pixel around it.
float3 DnViewDirection(float2 viewPosition)
{
    float2 uv = viewPosition * g_DnViewportSize.zw;
    float4 farPoint = mul(float4(uv.x * 2.0 - 1.0, 1.0 - 2.0 * uv.y, 1.0, 1.0), g_DnInvViewProj);
    return farPoint.xyz / farPoint.w - g_DnCameraPos.xyz;
}

float DnBrightness(float4 value)
{
    return dot(value.rgb, float3(0.2126, 0.7152, 0.0722));
}

// True where the signal has nothing to clean up: the sky, and for reflections surfaces too rough to get a ray
bool DnIsEmpty(float4 guide)
{
    return guide.w <= 0.0 || (g_DnKind == SIGNAL_REFLECTION && guide.w >= g_DnMaxRoughness);
}

float4 DnSanitize(float4 value)
{
    return (any(isnan(value)) || any(isinf(value))) ? float4(0.0, 0.0, 0.0, 0.0) : value;
}

#endif // DEFERRED_HYBRID_DENOISE_COMMON_HLSLI
