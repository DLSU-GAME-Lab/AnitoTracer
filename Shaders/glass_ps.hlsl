// glass_ps.hlsl
// Transparent pass of the ray-traced renderers (Hybrid, and Deferred Hybrid after its lighting pass): draws the blended
// materials, farthest first, over the lit scene. Each pixel shades itself like a forward pixel (several rays per light,
// ambient occlusion, a reflection ray) and adds the view through the glass: see ShadeGlassSurface in hybrid_shading.hlsli.
#include "common_struct.hlsli"
#include "light_struct.hlsli"
#include "pbr_defs.hlsli"
#include "material_sampling.hlsli"
#include "fog.hlsli"
#include "hybrid_shading.hlsli"

void main_ps(in PSInput In, out float4 OutColor : SV_TARGET)
{
    // Angle between this pixel's view ray and its neighbours', for the mip of textures seen in reflections
    float pixelSpread = ViewRaySpread(In.WorldPos, g_CameraPos.xyz);

    float4 albedo = SampleBaseColor(In.UV);

    // A cutout that lets light through is still cut out (a blended material has no cutoff)
    if (!PassesAlphaTest(albedo.a))
    {
        discard;
    }

    float metallic;
    float roughness;
    SampleMetallicRoughness(In.UV, metallic, roughness);

    float3 geoN = normalize(In.Normal);
    float3 N = ApplyNormalMap(In, geoN);
    roughness = FilterRoughnessByNormal(N, roughness);

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
    surface.Layers = ReadMaterialLayers(geoN);
    surface.PixelSpread = pixelSpread;
    // Glass has no G-buffer entry, so it is lit with Sky + AO whichever GI method the scene uses
    surface.UseIndirectLight = false;
    surface.IndirectLight = float4(0.0, 0.0, 0.0, 1.0);

    // A metal reflects all the light that reaches it: only the dielectric part of a surface lets any through
    float transmission = g_MaterialParams.z * (1.0 - metallic);
    OutColor = ShadeGlassSurface(surface, SurfaceOpacity(albedo.a), g_MaterialParams.y, transmission, In.Pos.xy);
}
