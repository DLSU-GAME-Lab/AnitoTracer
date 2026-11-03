#ifndef DEFERRED_HYBRID_GBUFFER_HLSLI
#define DEFERRED_HYBRID_GBUFFER_HLSLI

// Deferred Hybrid G-buffer layout (must match DeferredHybridPipeline.cpp):
//   RT0 RGBA8_UNORM_SRGB : albedo.rgb, metallic (alpha is stored linear)
//   RT1 RGBA16_FLOAT     : octahedral shading normal (xy), octahedral geometric normal (zw)
//   RT2 RGBA8_UNORM_SRGB : emissive.rgb, material AO (alpha is stored linear)
//   RT3 R8_UNORM         : roughness
//   Depth D32_FLOAT      : world position is reconstructed from this, no position target needed

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
