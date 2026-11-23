// reflection_trace_ps.hlsl
// Deferred Hybrid reflection pass. Paired with Deferred/fullscreen_vs.hlsl.
//
// One reflection ray for every visible pixel smooth enough to get one (UsesReflectionRay). Writes the light arriving
// along the reflection (rgb) and how far away its source is (a), see TraceHybridReflection. On a rough surface each
// frame's ray picks one direction out of the highlight's lobe, so the result is noise until the denoiser has
// averaged it; the hit distance tells the denoiser how wide a blur that surface's reflection can take.
//
// Runs after the indirect signal is ready: what a ray hits is lit by the indirect light gathered at this pixel.
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../fog.hlsli"
#include "../hybrid_shading.hlsli"
#include "deferred_common.hlsli"

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    int2 pixel = int2(In.Pos.xy);

    HybridSurface surface;
    float depth;
    if (!LoadGBufferSurface(pixel, surface, depth) || !UsesReflectionRay(surface.Roughness))
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }

    float ambientVisibility;
    float3 openDirection;
    ApplyIndirectSignal(surface, pixel, ambientVisibility, openDirection);
    HybridAmbient ambient = GetHybridAmbient(surface, ambientVisibility, openDirection);

    float2 noise = SignalNoise(In.Pos.xy, DENOISE_REFLECTIONS);

    float4 reflection = TraceHybridReflection(surface, ambient, noise);
    return float4(SanitizeLight(reflection.rgb), max(reflection.a, 0.0));
}
