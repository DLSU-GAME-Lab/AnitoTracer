// gbuffer_ps.hlsl
// Deferred Hybrid geometry pass: samples the material and packs it into the G-buffer. No lighting or rays here;
// the lighting pass (lighting_ps.hlsl) traces once per visible pixel.
#include "../common_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "gbuffer_common.hlsli"

struct GBufferOutput
{
    float4 AlbedoMetallic : SV_TARGET0;
    float4 Normals : SV_TARGET1;
    float4 EmissiveAO : SV_TARGET2;
    float Roughness : SV_TARGET3;
};

void main_ps(in PSInput In, out GBufferOutput Out)
{
    // Material sampling, identical to hybrid_ps.hlsl
    float4 albedo = g_BaseColorFactor;
    if (g_UseBaseColorMap > 0.5)
    {
        albedo *= g_BaseColorMap.Sample(g_BaseColorMap_sampler, In.UV);
    }

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

    // The geometric normal is kept alongside the shading normal: rays are offset along it to avoid self-shadowing
    float3 geoN = normalize(In.Normal);

    float3 N = geoN;
    float3 T = normalize(In.Tangent);
    float3 B = normalize(In.Bitangent);

    float3x3 TBN = float3x3(T, B, N);

    if (g_UseNormalMap > 0.5)
    {
        float3 tangentNormal = g_NormalMap.Sample(g_NormalMap_sampler, In.UV).xyz * 2.0 - 1.0;
        N = normalize(mul(tangentNormal, TBN));
    }

    float3 emissive = float3(0.0, 0.0, 0.0);
    if (g_UseEmissiveMap > 0.5)
    {
        emissive = g_EmissiveMap.Sample(g_EmissiveMap_sampler, In.UV).rgb;
    }

    Out.AlbedoMetallic = float4(albedo.rgb, metallic);
    Out.Normals = float4(OctEncode(N), OctEncode(geoN));
    Out.EmissiveAO = float4(emissive, ao);
    Out.Roughness = roughness;
}
