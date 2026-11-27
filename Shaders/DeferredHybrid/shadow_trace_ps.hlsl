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

struct ShadowOutput
{
    float4 Visibility : SV_TARGET0;
#ifdef GLASS_TINT_OUTPUT
    // rgb: the color glass gives this pixel's direct light, white where none is in the way
    float4 Tint : SV_TARGET1;
#endif
};

void main_ps(in FullScreenPSInput In, out ShadowOutput Out)
{
    Out.Visibility = float4(1.0, 1.0, 1.0, 1.0);
#ifdef GLASS_TINT_OUTPUT
    Out.Tint = float4(1.0, 1.0, 1.0, 1.0);
#endif

    HybridSurface surface;
    float depth;
    if (!LoadGBufferSurface(int2(In.Pos.xy), surface, depth))
    {
        return;
    }

    float2 noise = SignalNoise(In.Pos.xy, DENOISE_SHADOWS);

    int firstSlot = int(g_ShadowLayer) * 4;
    int slotCount = LightSlotCount();
    float3 tintSum = float3(0.0, 0.0, 0.0);
    float tintWeight = 0.0;
    for (int channel = 0; channel < 4; ++channel)
    {
        int slot = firstSlot + channel;
        if (slot < slotCount)
        {
            // A light's visibility includes the brightness glass takes from it; the glass's color is kept apart
            float3 chroma;
            Out.Visibility[channel] = TraceLightSlot(surface, slot, noise, 1, chroma);

            float weight = LightTintWeight(surface, slot) * Out.Visibility[channel];
            tintSum += chroma * weight;
            tintWeight += weight;
        }
    }

#ifdef GLASS_TINT_OUTPUT
    if (tintWeight > 0.00001)
    {
        Out.Tint = float4(tintSum / tintWeight, 1.0);
    }
#endif
}
