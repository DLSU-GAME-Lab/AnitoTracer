// guide_ps.hlsl
// Deferred Hybrid guide pass. Paired with Deferred/fullscreen_vs.hlsl.
//
// Writes what the denoisers need to know about each pixel's surface, in one texture: its geometric normal, its
// distance from the camera and its roughness. Two pixels with similar values lie on the same surface, so their rays
// may be averaged; where they differ there is an edge the blur must stop at. The pipeline keeps last frame's guide
// too: comparing the two tells whether a pixel still shows what it showed a frame ago.
//
// The geometric normal is used, not the normal mapped one: a normal map's fine detail would make neighbouring
// pixels look like different surfaces and stop the blur from averaging them.
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../fog.hlsli"
#include "../hybrid_shading.hlsli"
#include "deferred_common.hlsli"

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    int3 texel = int3(In.Pos.xy, 0);

    // Sky: w = 0 tells every later pass to leave this pixel out
    float depth = g_GBufferDepth.Load(texel).r;
    if (depth >= 1.0)
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }

    float2 uv = In.Pos.xy * g_ViewportSize.zw;
    float4 worldPos = mul(float4(uv.x * 2.0 - 1.0, 1.0 - 2.0 * uv.y, depth, 1.0), g_InvViewProj);
    float viewDistance = length(worldPos.xyz / worldPos.w - g_ViewOrigin.xyz);

    // Roughness is never 0 on a surface (the G-buffer pass keeps it above 0.05), but the sky marker must not depend on that
    float roughness = max(g_GBufferRoughness.Load(texel).r, 1.0 / 255.0);
    return float4(g_GBufferNormals.Load(texel).zw, viewDistance, roughness);
}
