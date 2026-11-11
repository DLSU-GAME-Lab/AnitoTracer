// lighting_ps.hlsl
// Deferred Hybrid lighting pass: one fullscreen triangle, rebuilds each visible surface from the G-buffer
// and runs the shared ray-traced shading on it, so rays are traced exactly once per pixel.
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../fog.hlsli"
#include "../hybrid_shading.hlsli"
#include "gbuffer_common.hlsli"

Texture2D g_GBufferAlbedoMetallic;
Texture2D g_GBufferNormals;
Texture2D g_GBufferEmissiveAO;
Texture2D g_GBufferRoughness;
Texture2D g_GBufferDepth;

cbuffer DeferredHybridConstants
{
    float4x4 g_InvViewProj;
    float4 g_ViewportSize; // xy: size in pixels, zw: 1 / size
};

struct FullScreenPSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

struct LightingOutput
{
    float4 Color : SV_TARGET;
    float Depth : SV_DEPTH; // Copies scene depth into the target so particles and debug lines are occluded
};

void main_ps(in FullScreenPSInput In, out LightingOutput Out)
{
    // The G-buffer can be larger than the viewport (it only grows), so fetch by pixel instead of by UV
    int3 texel = int3(In.Pos.xy, 0);

    // Nothing was drawn here; keep the skybox
    float depth = g_GBufferDepth.Load(texel).r;
    if (depth >= 1.0)
    {
        discard;
    }

    // Reconstruct world position from depth
    float2 uv = In.Pos.xy * g_ViewportSize.zw;
    float4 clipPos = float4(uv.x * 2.0 - 1.0, 1.0 - 2.0 * uv.y, depth, 1.0);
    float4 worldPos = mul(clipPos, g_InvViewProj);
    worldPos.xyz /= worldPos.w;

    // D32 depth loses precision with distance, so the rebuilt position can sit below the real surface by more
    // than the shadow bias (tenths of a unit a few hundred units out). Two depth steps, mapped back to world
    // units, is how far it can be off here; the ray origins are pushed out by that much on top of the bias.
    float depthStep = asfloat(asuint(depth) + 2) - depth;
    float4 worldPosNext = mul(float4(clipPos.xy, depth + depthStep, 1.0), g_InvViewProj);
    float positionError = length(worldPosNext.xyz / worldPosNext.w - worldPos.xyz);

    float4 albedoMetallic = g_GBufferAlbedoMetallic.Load(texel);
    float4 normals = g_GBufferNormals.Load(texel);
    float4 emissiveAO = g_GBufferEmissiveAO.Load(texel);

    HybridSurface surface;
    surface.WorldPos = worldPos.xyz;
    surface.N = OctDecode(normals.xy);
    surface.GeoN = OctDecode(normals.zw);
    surface.PositionError = positionError;
    surface.Albedo = albedoMetallic.rgb;
    surface.Metallic = albedoMetallic.a;
    surface.Roughness = g_GBufferRoughness.Load(texel).r;
    surface.AO = emissiveAO.a;
    surface.Emissive = emissiveAO.rgb;

    Out.Color = float4(ShadeHybridSurface(surface, In.Pos.xy), 1.0);
    Out.Depth = depth;
}
