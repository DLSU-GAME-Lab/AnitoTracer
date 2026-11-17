// lighting_ps.hlsl
// Deferred Hybrid lighting pass: one fullscreen triangle that composes the final color of each visible surface from
// the G-buffer (its material) and the ray-traced signals (what its rays found, see deferred_common.hlsli).
// It traces no rays itself: the trace passes did, once per pixel, and the denoisers cleaned the results up.
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../fog.hlsli"
#include "../hybrid_shading.hlsli"
#include "deferred_common.hlsli"

// Light visibility, four light slots per texture (r, g, b, a): slots 0-3, 4-7 and 8-11.
// A white texture is bound for the ones the scene has no lights for.
Texture2D g_ShadowSignal0;
Texture2D g_ShadowSignal1;
Texture2D g_ShadowSignal2;

// rgb: light arriving along the reflection, a: distance to its source. Only read where UsesReflectionRay.
Texture2D g_ReflectionSignal;

struct LightingOutput
{
    float4 Color : SV_TARGET;
    float Depth : SV_DEPTH; // Copies scene depth into the target so particles and debug lines are occluded
};

void main_ps(in FullScreenPSInput In, out LightingOutput Out)
{
    int2 pixel = int2(In.Pos.xy);
    int3 texel = int3(pixel, 0);

    // Nothing was drawn here; keep the skybox
    HybridSurface surface;
    float depth;
    if (!LoadGBufferSurface(pixel, surface, depth))
    {
        discard;
    }

    HybridRays rays;
    ApplyIndirectSignal(surface, pixel, rays.AmbientVisibility, rays.OpenDirection);

    int slotCount = LightSlotCount();
    float4 shadows0 = g_ShadowSignal0.Load(texel);
    float4 shadows1 = float4(1.0, 1.0, 1.0, 1.0);
    float4 shadows2 = float4(1.0, 1.0, 1.0, 1.0);
    if (slotCount > 4) shadows1 = g_ShadowSignal1.Load(texel);
    if (slotCount > 8) shadows2 = g_ShadowSignal2.Load(texel);
    for (int channel = 0; channel < 4; ++channel)
    {
        rays.LightVisibility[channel] = saturate(shadows0[channel]);
        rays.LightVisibility[4 + channel] = saturate(shadows1[channel]);
        rays.LightVisibility[8 + channel] = saturate(shadows2[channel]);
    }

    rays.Reflection = float3(0.0, 0.0, 0.0);
    if (UsesReflectionRay(surface.Roughness))
    {
        rays.Reflection = SanitizeLight(g_ReflectionSignal.Load(texel).rgb);
    }

    Out.Color = float4(ComposeHybridSurface(surface, rays, In.Pos.xy), 1.0);
    Out.Depth = depth;
}
