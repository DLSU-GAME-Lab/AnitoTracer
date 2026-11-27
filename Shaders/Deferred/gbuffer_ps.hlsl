#include "../common_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../material_sampling.hlsli"

struct GBufferOutput
{
    float4 AlbedoMetallic : SV_TARGET0;
    float4 NormalRoughness : SV_TARGET1;
    float4 WorldPos : SV_TARGET2;
    float4 Emissive : SV_TARGET3;
};

void main_ps(in PSInput In, out GBufferOutput Out)
{
    // 1. Material Sampling
    float4 albedo = SampleBaseColor(In.UV);
    
    if (!PassesAlphaTest(albedo.a))
    {
        discard;
    }

    float metallic;
    float roughness;
    SampleMetallicRoughness(In.UV, metallic, roughness);

    // Note: AO is skipped here; it would need another G-Buffer channel.
    float3 emissive = SampleEmissive(In.UV);


    float3 geoN = normalize(In.Normal);
    float3 N = ApplyNormalMap(In, geoN);
    roughness = FilterRoughnessByNormal(N, roughness);

    // Pack data into the G-Buffer
    Out.AlbedoMetallic = float4(albedo.rgb, metallic);
    Out.NormalRoughness = float4(N, roughness);
    
    // Storing 1.0 in the W channel to signify a valid rendered pixel
    // Your deferred light shader will discard background pixels if w == 0.0
    Out.WorldPos = float4(In.WorldPos, 1.0);
    Out.Emissive = float4(emissive, 1.0);
}