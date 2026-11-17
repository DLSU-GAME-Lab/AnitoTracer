// shadow_trace_ps.hlsl
// Deferred Hybrid shadow pass. Paired with Deferred/fullscreen_vs.hlsl.
//
// One shadow ray per light for every visible pixel, aimed at a random point of the light's area. The result is
// 0 (that point is hidden) or 1 (it is visible), one light per color channel. Averaged over frames and neighbours
// by the denoiser this becomes a soft shadow; a light's four rays per pixel used to do that on their own.
//
// A texture holds four lights, so the pass is drawn once per four lights in the scene (g_ShadowLayer).
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../fog.hlsli"
#include "../hybrid_shading.hlsli"
#include "deferred_common.hlsli"

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    float4 visibility = float4(1.0, 1.0, 1.0, 1.0);

    HybridSurface surface;
    float depth;
    if (!LoadGBufferSurface(int2(In.Pos.xy), surface, depth))
    {
        return visibility;
    }

    float2 noise = SignalNoise(In.Pos.xy, DENOISE_SHADOWS);

    int firstSlot = int(g_ShadowLayer) * 4;
    int slotCount = LightSlotCount();
    for (int channel = 0; channel < 4; ++channel)
    {
        int slot = firstSlot + channel;
        if (slot < slotCount)
        {
            visibility[channel] = TraceLightSlot(surface, slot, noise, 1);
        }
    }
    return visibility;
}
