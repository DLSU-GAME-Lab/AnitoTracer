// anime_ps.hlsl
// Anime (cel) shading, ported from Weeb_Big2's fs_weeb.sc:
//   * two tones, the lit one and the shadow one, switched by a hard step on N.L ("Step")
//   * optional Cook-Torrance specular on top (the original's PBR mode)
//   * ink along the inner edges: where a surface turns away from the camera ("Inner Step") and where the mesh bends
//     sharply ("Curve Step", from the per-vertex curvature in the second vertex stream)
//   * smooth normals for the band (Model::pAnimeVertexBuffer), and blurred base color / normal map textures
// The outline hull is drawn by outline_vs/ps.hlsl.
#include "../common_struct.hlsli"
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../fog.hlsli"
#include "anime_common.hlsli"

// ------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------

// 1 where signedDistance is positive, 0 where it is negative, with a one pixel soft edge between so tone bands and
// ink lines don't crawl. Uses derivatives, so call it from uniform control flow.
float SoftEdge(float signedDistance)
{
    return saturate(signedDistance / max(fwidth(signedDistance), 1e-5) + 0.5);
}

// The texture blur settings are mip levels: the texture is read from at least that mip
float4 SampleBaseColor(float2 uv)
{
    if (g_AnimeBaseColorBlur > 0.0)
    {
        float lod = max(g_BaseColorMap.CalculateLevelOfDetail(g_BaseColorMap_sampler, uv), g_AnimeBaseColorBlur);
        return g_BaseColorMap.SampleLevel(g_BaseColorMap_sampler, uv, lod);
    }
    return g_BaseColorMap.Sample(g_BaseColorMap_sampler, uv);
}

float3 SampleTangentNormal(float2 uv)
{
    float3 encoded;
    if (g_AnimeNormalMapBlur > 0.0)
    {
        float lod = max(g_NormalMap.CalculateLevelOfDetail(g_NormalMap_sampler, uv), g_AnimeNormalMapBlur);
        encoded = g_NormalMap.SampleLevel(g_NormalMap_sampler, uv, lod).xyz;
    }
    else
    {
        encoded = g_NormalMap.Sample(g_NormalMap_sampler, uv).xyz;
    }
    return encoded * 2.0 - 1.0;
}

struct AnimeLighting
{
    float3 tint;        // Light reaching the lit tone, summed over the lights
    float amount;       // How much of the surface the lit tone covers; the rest is the shadow tone
    float3 specular;
};

// A light's share of the surface. radiance is color * intensity, attenuation 1 for directional lights.
void AddLight(inout AnimeLighting acc, float3 L, float3 radiance, float attenuation,
    float3 Nband, float3 Nshade, float3 V, float roughness, float3 F0)
{
    // Display colors: the light is a tint for the lit tone, so intensities above 1 don't blow it out
    radiance /= max(1.0, max(radiance.r, max(radiance.g, radiance.b)));

    // The band: lit tone where the surface faces the light by more than Step
    float toon = SoftEdge(dot(Nband, L) - g_AnimeStep);
    float NdotL = saturate(dot(Nshade, L));
    float weight = toon * attenuation * lerp(1.0, NdotL, g_AnimeLightFalloff);

    acc.tint += radiance * weight;
    acc.amount += weight;

    if (g_AnimeSpecularEnabled != 0 && toon > 0.0 && NdotL > 0.0)
    {
        float3 H = normalize(V + L);
        float NDF = DistributionGGX(Nshade, H, roughness);
        float G = GeometrySmith(Nshade, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        float NdotV = max(dot(Nshade, V), 0.0);
        float3 spec = NDF * G * F / (4.0 * NdotV * NdotL + 0.0001);

        // The tones carry no 1/PI, so the specular lobe is scaled by PI to match
        acc.specular += spec * radiance * (PI * attenuation * toon * NdotL);
    }
}

// ------------------------------------------------------------------
// Main
// ------------------------------------------------------------------

void main_ps(in AnimePSInput In, out float4 OutColor : SV_TARGET)
{
    // 1. Material
    float4 albedo = g_BaseColorFactor;
    if (g_UseBaseColorMap > 0.5)
    {
        albedo *= SampleBaseColor(In.UV);
    }

    float roughness = g_RoughnessFactor;
    float metallic = g_MetallicFactor;
    if (g_UseMetallicRoughnessMap > 0.5)
    {
        ApplyMetallicRoughnessMap(g_MetallicRoughnessMap.Sample(g_MetallicRoughnessMap_sampler, In.UV), metallic, roughness);
    }
    roughness = clamp(roughness, 0.05, 1.0);

    // 2. Normals. The band is cut with the mesh's (or the smooth) normal, so a normal map doesn't turn the terminator
    // into noise; the normal map shapes the specular and the falloff only.
    float3 geoN = normalize(In.Normal);
    float3 Nband = geoN;
    if (g_AnimeUseSmoothNormal != 0)
    {
        Nband = normalize(In.SmoothNormal);
    }

    float3 Nshade = Nband;
    if (g_UseNormalMap > 0.5)
    {
        float3x3 TBN = float3x3(normalize(In.Tangent), normalize(In.Bitangent), Nband);
        Nshade = normalize(mul(SampleTangentNormal(In.UV), TBN));
    }

    float3 V = normalize(g_CameraPos.xyz - In.WorldPos);

    float3 litAlbedo = albedo.rgb * g_AnimeBaseColor.rgb;
    float3 shadowAlbedo = albedo.rgb * g_AnimeShadowColor.rgb * (1.0 - g_AnimeShadowFactor);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), litAlbedo, metallic);

    // 3. Lights
    AnimeLighting lighting;
    lighting.tint = float3(0.0, 0.0, 0.0);
    lighting.amount = 0.0;
    lighting.specular = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < g_NumDirLights; ++i)
    {
        float3 L = normalize(-g_DirLights[i].Direction.xyz);
        float3 radiance = g_DirLights[i].Color.rgb * g_DirLights[i].Color.a;
        AddLight(lighting, L, radiance, 1.0, Nband, Nshade, V, roughness, F0);
    }

    // Out of range lights get an attenuation of 0 instead of being skipped, which keeps the derivatives in uniform flow
    for (int j = 0; j < g_NumPointLights; ++j)
    {
        float3 lightVec = g_PointLights[j].Position.xyz - In.WorldPos;
        float dist = length(lightVec);
        float3 L = lightVec / max(dist, 1e-4);

        float attenuation = saturate(1.0 - dist / max(g_PointLights[j].Range, 1e-4));
        attenuation *= attenuation;

        float3 radiance = g_PointLights[j].Color.rgb * g_PointLights[j].Color.a;
        AddLight(lighting, L, radiance, attenuation, Nband, Nshade, V, roughness, F0);
    }

    // 4. Tones: the shadow tone where no light reaches, the lit tone tinted by the lights where it does
    float litAmount = saturate(lighting.amount);
    float3 color = shadowAlbedo * (1.0 - litAmount)
        + litAlbedo * saturate(lighting.tint)
        + lighting.specular * g_AnimeSpecularStrength
        + SampleEmissive(In.UV);

    // 5. Inner edges: ink where a surface turns away from the camera, and where the mesh bends sharply
    float edge = 0.0;
    if (g_AnimeInnerStep > 0.0)
    {
        float facing = saturate(dot(geoN, V));
        edge = max(edge, SoftEdge(g_AnimeInnerStep - facing));
    }
    if (g_AnimeCurveStep < 1.0)
    {
        edge = max(edge, SoftEdge(In.Curvature - g_AnimeCurveStep));
    }
    color = lerp(color, g_AnimeInnerEdgeColor.rgb, edge);

    color = ApplyFog(color, In.WorldPos, In.Pos.xy);

    // Alpha testing for foliage and cutouts, after the derivatives above
    if (!PassesAlphaTest(albedo.a))
    {
        discard;
    }

    OutColor = float4(color, albedo.a);
}
