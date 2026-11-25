// gbuffer_ps.hlsl
// Deferred Hybrid geometry pass: samples the material and packs it into the G-buffer. No lighting or rays here;
// the lighting pass (lighting_ps.hlsl) traces once per visible pixel.
#include "../common_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../material_sampling.hlsli"
#include "gbuffer_common.hlsli"

struct GBufferOutput
{
    float4 AlbedoMetallic : SV_TARGET0;
    float4 Normals : SV_TARGET1;
    float3 Emissive : SV_TARGET2;
    float4 Material : SV_TARGET3;
    float2 Velocity : SV_TARGET4;
    float4 Sheen : SV_TARGET5;
};

void main_ps(in PSInput In, out GBufferOutput Out)
{
    // Material sampling, identical to hybrid_ps.hlsl
    float4 albedo = SampleBaseColor(In.UV);

    if (!PassesAlphaTest(albedo.a))
    {
        discard;
    }

    float metallic;
    float roughness;
    SampleMetallicRoughness(In.UV, metallic, roughness);

    float ao = SampleOcclusion(In.UV);

    // The geometric normal is kept alongside the shading normal: rays are offset along it to avoid self-shadowing
    float3 geoN = normalize(In.Normal);

    float3 N = ApplyNormalMap(In, geoN);

    // Highlights too small for a pixel to show would flicker: the roughness covers them
    roughness = FilterRoughnessByNormal(N, roughness);

    float3 emissive = SampleEmissive(In.UV);
    SurfaceLayers layers = ReadMaterialLayers(geoN);

    Out.AlbedoMetallic = float4(albedo.rgb, metallic);
    Out.Normals = float4(OctEncode(N), OctEncode(geoN));
    Out.Emissive = emissive;
    Out.Material = float4(roughness, ao, layers.Clearcoat, layers.ClearcoatRoughness);
    Out.Sheen = float4(saturate(layers.SheenColor), layers.SheenRoughness);

    // Motion vector, from where the vertex shader put this point this frame and last frame
    float2 velocity = float2(4.0, 4.0); // Was behind the camera: points far off screen, so nothing is reused
    if (In.PrevClip.w > 0.0001 && In.CurrClip.w > 0.0001)
    {
        velocity = (In.CurrClip.xy / In.CurrClip.w - In.PrevClip.xy / In.PrevClip.w) * float2(0.5, -0.5);
    }
    Out.Velocity = velocity;
}
