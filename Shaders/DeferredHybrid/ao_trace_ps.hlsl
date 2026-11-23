// ao_trace_ps.hlsl
// Deferred Hybrid ambient occlusion pass, for the GI method "Sky + AO". Paired with Deferred/fullscreen_vs.hlsl.
//
// One short ray per visible pixel in a random direction over the surface (more often near the normal). The result
// is where the ray went if nothing was in its way (rgb) and whether it got away (a); averaged by the denoiser these
// become the direction the surface is open toward and how open it is, which the lighting pass turns into sky light.
// Only needs the scene's acceleration structure, so it runs on every device that traces rays at all.
//
// With the project's Half-Resolution Occlusion on, the pass draws a texture half as wide and high: one ray for
// every 2x2 view pixels, from the surface in the top-left one. The lighting pass scales the result back up
// (SampleIndirectSignal).
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../fog.hlsli"
#include "../hybrid_shading.hlsli"
#include "deferred_common.hlsli"

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    HybridSurface surface;
    float depth;
    // The view pixel this signal pixel stands for, as the guide and the denoiser have it (g_GIParams.y: 1 or 2)
    int2 viewPixel = min(int2(In.Pos.xy) * max(int(g_GIParams.y), 1), int2(g_ViewportSize.xy) - 1);
    if (!LoadGBufferSurface(viewPixel, surface, depth))
    {
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    float2 noise = SignalNoise(In.Pos.xy, DENOISE_INDIRECT);

    float3 openDirection;
    float visibility = TraceAmbientOcclusion(surface.WorldPos, surface.GeoN, surface.PositionError, surface.N,
        ShiftNoise(noise, 4 + 8), 1, openDirection);
    return float4(openDirection, visibility);
}
