#include "../common_struct.hlsli"
#include "../pbr_defs.hlsi"

struct GBufferOutput
{
    float4 AlbedoMetallic : SV_TARGET0;
    float4 NormalRoughness : SV_TARGET1;
    float4 WorldPos : SV_TARGET2;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    centroid float3 WorldPos : TEXCOORD0;
    centroid float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
};

void main_ps(in PSInput In, out GBufferOutput Out)
{
    // 1. Material Sampling
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
        roughness *= mrSample.g;
        metallic *= mrSample.b;
    }
    roughness = max(roughness, 0.05);

    // Note: Emissive and AO are skipped here as they are typically applied 
    // in the lighting pass or require additional G-Buffer targets.
    
    float3 geoN = normalize(In.Normal);
    float3 N = geoN;
    float3 T = normalize(In.Tangent);
    float3 B = normalize(In.Bitangent);

    float3x3 TBN = float3x3(T, B, N);

    // --- Normal Mapping Logic (Derivative TBN) ---
    if (g_UseNormalMap > 0.5)
    {
        float3 tangentNormal = g_NormalMap.Sample(g_NormalMap_sampler, In.UV).xyz * 2.0 - 1.0;
        N = normalize(mul(tangentNormal, TBN));
    }

    // Pack data into the G-Buffer
    Out.AlbedoMetallic = float4(albedo.rgb, metallic);
    Out.NormalRoughness = float4(N, roughness);
    
    // Storing 1.0 in the W channel to signify a valid rendered pixel
    // Your deferred light shader will discard background pixels if w == 0.0
    Out.WorldPos = float4(In.WorldPos, 1.0);
}