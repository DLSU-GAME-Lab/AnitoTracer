#ifndef DEFERRED_HYBRID_GBUFFER_HLSLI
#define DEFERRED_HYBRID_GBUFFER_HLSLI

// Deferred Hybrid G-buffer layout (must match DeferredHybridPipeline.cpp: the single source of truth for what is where):
//   RT0 RGBA8_UNORM_SRGB : albedo.rgb, metallic (alpha is stored linear)
//   RT1 RGBA16_FLOAT     : octahedral shading normal (xy), octahedral geometric normal (zw)
//   RT2 R11G11B10_FLOAT  : emissive.rgb, HDR: a strength above 1 stays above 1 (up to 65000). No sign, no alpha.
//   RT3 RGBA8_UNORM      : roughness, material AO, clear coat strength, clear coat roughness
//   RT4 RG16_FLOAT       : motion vector: how far the surface moved across the screen since last frame, in UV units
//                          (uv - velocity is where it was). Camera and object motion together, without TAA jitter.
//   RT5 RGBA8_UNORM      : sheen color.rgb, sheen roughness (black: no sheen)
//   Depth D32_FLOAT      : world position is reconstructed from this, no position target needed
// The clear coat follows the geometric normal (RT1.zw), so it needs no normal of its own.
// Transmissive materials are not here: they are drawn with the glass, after the lighting pass (glass_ps.hlsl).

// Octahedral normal encoding (Cigolle et al. 2014): a unit vector in two values
float2 OctWrap(float2 v)
{
    return (1.0 - abs(v.yx)) * float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

float2 OctEncode(float3 n)
{
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    return n.z >= 0.0 ? n.xy : OctWrap(n.xy);
}

float3 OctDecode(float2 e)
{
    float3 n = float3(e.x, e.y, 1.0 - abs(e.x) - abs(e.y));
    float t = saturate(-n.z);
    n.xy += float2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);
    return normalize(n);
}

#endif // DEFERRED_HYBRID_GBUFFER_HLSLI
