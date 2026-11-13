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
    float4 g_ViewParams;   // x: angle one pixel covers, in radians
    float4 g_GIParams;     // x: 1 when per-pixel GI is on, y: view pixels per GI pixel (1 or 2), z: GI intensity
};

// Per-pixel diffuse GI from DiffuseGIPass (see gi_common.hlsli). Dummies are bound while it is off.
Texture2D g_GIResult;   // rgb: indirect light arriving, a: how open the surroundings are
Texture2D g_GIGuide;    // xy: octahedral geometric normal, z: distance from the camera, w: 1 where there is a surface

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

// The GI for a view pixel. At half resolution each GI pixel covers 2x2 view pixels, so the four around this pixel
// are blended, leaving out the ones that lie on another surface (the guide tells): simply stretching the GI
// would smear light across object edges.
float4 SampleIndirectLight(int2 pixel, float3 geoN, float viewDistance)
{
    int scale = max(int(g_GIParams.y), 1);
    int2 base = pixel / scale;
    float2 f = float2(pixel - base * scale) / float(scale);
    int2 maxTexel = (int2(g_ViewportSize.xy) + scale - 1) / scale - 1;

    float4 sum = float4(0.0, 0.0, 0.0, 0.0);
    float weightSum = 0.0;
    for (int j = 0; j < 2; ++j)
    {
        for (int i = 0; i < 2; ++i)
        {
            float w = (i == 0 ? 1.0 - f.x : f.x) * (j == 0 ? 1.0 - f.y : f.y);
            if (w <= 0.0)
                continue;

            int2 texel = min(base + int2(i, j), maxTexel);
            float4 guide = g_GIGuide.Load(int3(texel, 0));
            w *= guide.w;
            w *= pow(saturate(dot(geoN, OctDecode(guide.xy))), 8.0);
            w *= exp(-abs(guide.z - viewDistance) / (0.05 * viewDistance + 0.01));

            sum += g_GIResult.Load(int3(texel, 0)) * w;
            weightSum += w;
        }
    }

    // None of the four matched (a thin object between GI pixels): the nearest one is the best guess left
    if (weightSum < 0.0001)
    {
        return g_GIResult.Load(int3(min(base, maxTexel), 0));
    }
    return sum / weightSum;
}

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
    surface.PixelSpread = g_ViewParams.x;

    surface.UseIndirectLight = g_GIParams.x > 0.5;
    surface.IndirectLight = float4(0.0, 0.0, 0.0, 1.0);
    if (surface.UseIndirectLight)
    {
        surface.IndirectLight = SampleIndirectLight(texel.xy, surface.GeoN, length(worldPos.xyz - g_CameraPos.xyz));
        surface.IndirectLight.rgb *= g_GIParams.z;
    }



    Out.Color = float4(ShadeHybridSurface(surface, In.Pos.xy), 1.0);
    Out.Depth = depth;
}
