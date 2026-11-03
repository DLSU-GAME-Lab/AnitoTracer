// hybrid_ps.hlsl
// Forward Hybrid pass: samples the material, then runs the shared ray-traced shading in hybrid_shading.hlsli
#include "common_struct.hlsli"
#include "light_struct.hlsli"
#include "pbr_defs.hlsli"
#include "fog.hlsli"
#include "hybrid_shading.hlsli"

void main_ps(in PSInput In, out float4 OutColor : SV_TARGET)
{
    // 1. Material Sampling
    float4 albedo = g_BaseColorFactor;
    if (g_UseBaseColorMap > 0.5)
    {
        albedo *= g_BaseColorMap.Sample(g_BaseColorMap_sampler, In.UV);
    }

    // Alpha testing for discarded fragments
    if (albedo.a < 0.5)
    {
        discard;
    }

    float metallic = g_MetallicFactor;
    float roughness = g_RoughnessFactor;
    if (g_UseMetallicRoughnessMap > 0.5)
    {
        float4 mrSample = g_MetallicRoughnessMap.Sample(g_MetallicRoughnessMap_sampler, In.UV);
        ApplyMetallicRoughnessMap(mrSample, metallic, roughness);
    }
    roughness = max(roughness, 0.05);

    float ao = 1.0;
    if (g_UseAOMap > 0.5)
    {
        ao = g_AOMap.Sample(g_AOMap_sampler, In.UV).r;
    }

    // Unperturbed geometric normal used strictly for ray origin offsets!
    float3 geoN = normalize(In.Normal);

    float3 N = geoN;
    float3 T = normalize(In.Tangent);
    float3 B = normalize(In.Bitangent);

    float3x3 TBN = float3x3(T, B, N);

    // Normal mapping
    if (g_UseNormalMap > 0.5)
    {
        float3 tangentNormal = g_NormalMap.Sample(g_NormalMap_sampler, In.UV).xyz * 2.0 - 1.0;
        N = normalize(mul(tangentNormal, TBN));
    }

    // Emissive Sampling
    float3 emissive = SampleEmissive(In.UV);

    // 2. Ray-traced shading
    HybridSurface surface;
    surface.WorldPos = In.WorldPos;
    surface.N = N;
    surface.GeoN = geoN;
    surface.PositionError = 0.0; // Interpolated position is exact
    surface.Albedo = albedo.rgb;
    surface.Metallic = metallic;
    surface.Roughness = roughness;
    surface.AO = ao;
    surface.Emissive = emissive;

    OutColor = float4(ShadeHybridSurface(surface, In.Pos.xy), albedo.a);
}
