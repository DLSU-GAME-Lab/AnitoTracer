// hybrid_ps.hlsl
// Forward Hybrid pass: samples the material, then runs the shared ray-traced shading in hybrid_shading.hlsli
#include "common_struct.hlsli"
#include "light_struct.hlsli"
#include "pbr_defs.hlsli"
#include "material_sampling.hlsli"
#include "fog.hlsli"
#include "hybrid_shading.hlsli"

void main_ps(in PSInput In, out float4 OutColor : SV_TARGET)
{
    // Angle between this pixel's view ray and its neighbours', for the mip of textures seen in reflections.
    // Taken before any discard: a derivative needs all four pixels of the 2x2 block to still be running.
    float pixelSpread = ViewRaySpread(In.WorldPos, g_CameraPos.xyz);

    // 1. Material Sampling
    float4 albedo = SampleBaseColor(In.UV);

    // Cutout materials discard the pixels below their alpha cutoff
    if (!PassesAlphaTest(albedo.a))
    {
        discard;
    }

    float metallic;
    float roughness;
    SampleMetallicRoughness(In.UV, metallic, roughness);

    float ao = SampleOcclusion(In.UV);

    // Unperturbed geometric normal used strictly for ray origin offsets!
    float3 geoN = normalize(In.Normal);

    // Normal mapping
    float3 N = ApplyNormalMap(In, geoN);

    // Highlights too small for a pixel to show would flicker: the roughness covers them
    roughness = FilterRoughnessByNormal(N, roughness);

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
    surface.Layers = ReadMaterialLayers(geoN);
    surface.PixelSpread = pixelSpread;
    // Per-pixel GI needs a G-buffer, so the forward pass always lights with Sky + AO
    surface.UseIndirectLight = false;
    surface.IndirectLight = float4(0.0, 0.0, 0.0, 1.0);



    OutColor = float4(ShadeHybridSurface(surface, In.Pos.xy), 1.0);
}
