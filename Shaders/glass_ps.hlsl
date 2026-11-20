// glass_ps.hlsl
// Transparent pass of the ray-traced renderers (Hybrid, and Deferred Hybrid after its lighting pass): draws the blended
// materials, farthest first, over the lit scene. Each pixel shades itself like a forward pixel (several rays per light,
// ambient occlusion, a reflection ray) and adds the view through the glass: see ShadeGlassSurface in hybrid_shading.hlsli.
#include "common_struct.hlsli"
#include "light_struct.hlsli"
#include "pbr_defs.hlsli"
#include "fog.hlsli"
#include "hybrid_shading.hlsli"

void main_ps(in PSInput In, out float4 OutColor : SV_TARGET)
{
    // Angle between this pixel's view ray and its neighbours', for the mip of textures seen in reflections
    float3 viewRay = normalize(In.WorldPos - g_CameraPos.xyz);
    float pixelSpread = max(length(ddx(viewRay)), length(ddy(viewRay)));

    float4 albedo = g_BaseColorFactor;
    if (g_UseBaseColorMap > 0.5)
    {
        albedo *= g_BaseColorMap.Sample(g_BaseColorMap_sampler, In.UV);
    }

    float metallic = g_MetallicFactor;
    float roughness = g_RoughnessFactor;
    if (g_UseMetallicRoughnessMap > 0.5)
    {
        float4 mrSample = g_MetallicRoughnessMap.Sample(g_MetallicRoughnessMap_sampler, In.UV);
        ApplyMetallicRoughnessMap(mrSample, metallic, roughness);
    }
    roughness = max(roughness, 0.05);

    float3 geoN = normalize(In.Normal);
    float3 N = geoN;
    if (g_UseNormalMap > 0.5)
    {
        float3x3 TBN = float3x3(normalize(In.Tangent), normalize(In.Bitangent), N);
        float3 tangentNormal = g_NormalMap.Sample(g_NormalMap_sampler, In.UV).xyz * 2.0 - 1.0;
        N = normalize(mul(tangentNormal, TBN));
    }

    HybridSurface surface;
    surface.WorldPos = In.WorldPos;
    surface.N = N;
    surface.GeoN = geoN;
    surface.PositionError = 0.0;
    surface.Albedo = albedo.rgb;
    surface.Metallic = metallic;
    surface.Roughness = roughness;
    surface.AO = 1.0;
    surface.Emissive = SampleEmissive(In.UV);
    surface.PixelSpread = pixelSpread;
    // Glass has no G-buffer entry, so it is lit with Sky + AO whichever GI method the scene uses
    surface.UseIndirectLight = false;
    surface.IndirectLight = float4(0.0, 0.0, 0.0, 1.0);

    OutColor = ShadeGlassSurface(surface, saturate(albedo.a), g_MaterialParams.y, In.Pos.xy);
}
