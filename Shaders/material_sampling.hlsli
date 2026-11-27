#ifndef MATERIAL_SAMPLING_HLSLI
#define MATERIAL_SAMPLING_HLSLI

// Reading a material at a pixel: the steps every pixel shader that draws models starts with (hybrid_ps, glass_ps,
// fallbackLit_ps, CrossHatch, the G-buffer passes of Deferred and Deferred Hybrid, the depth prepass), so they all
// read the same material the same way.
// Requires common_struct.hlsli (PSInput) and pbr_defs.hlsli (the material's constants and textures) to be included first.

// Base color: the material's factor times its map. Its alpha makes cutouts (see PassesAlphaTest) and is the opacity
// of glass.
float4 SampleBaseColor(float2 uv)
{
    float4 albedo = g_BaseColorFactor;
    if (g_UseBaseColorMap > 0.5)
    {
        albedo *= g_BaseColorMap.Sample(g_BaseColorMap_sampler, uv);
    }
    return albedo;
}

// Metallic and roughness: the material's factors times its map, except where the model component overrides one (see
// ApplyMetallicRoughnessMap). Roughness is kept above 0.05, since a perfect mirror has no highlight to speak of.
void SampleMetallicRoughness(float2 uv, out float metallic, out float roughness)
{
    metallic = g_MetallicFactor;
    roughness = g_RoughnessFactor;
    if (g_UseMetallicRoughnessMap > 0.5)
    {
        float4 mrSample = g_MetallicRoughnessMap.Sample(g_MetallicRoughnessMap_sampler, uv);
        ApplyMetallicRoughnessMap(mrSample, metallic, roughness);
    }
    roughness = max(roughness, 0.05);
}

// Specular anti-aliasing: where the shading normal changes quickly from pixel to pixel (a normal map's fine detail, a
// curved surface far away), the tiny highlights each of its normals makes are too small for a pixel to show, and it
// flickers between them as the camera moves. A rougher surface averages them: this adds the roughness that does.
// The normal's change across the pixel (ddx, ddy) gives the variance of the normals the pixel covers, and the GGX lobe
// widens by it (Tokuyoshi and Kaplanyan, "Improved Geometric Specular Antialiasing", I3D 2019: the screen space form,
// with the paper's filter variance and its cap on how far a lobe can widen).
// Strength 0 (the project's switch) leaves the roughness alone. N: the shading normal.
float FilterRoughnessByNormal(float3 N, float roughness)
{
    float strength = g_LayerParams.w;
    if (strength <= 0.0)
    {
        return roughness;
    }

    float3 dndx = ddx(N);
    float3 dndy = ddy(N);
    float variance = 0.15915494 * strength * (dot(dndx, dndx) + dot(dndy, dndy));
    float widening = min(2.0 * variance, 0.18);

    // Roughness is the square root of the GGX alpha: widen alpha^2 by the pixel's variance, then convert back
    float alpha = roughness * roughness;
    return sqrt(sqrt(saturate(alpha * alpha + widening)));
}

// How opaque a surface is as glass: the base color's alpha for a blended material. A material that is opaque and lets light
// through (its transmission) is drawn as glass too, and its alpha means nothing: it is 1.
float SurfaceOpacity(float baseAlpha)
{
    return g_MaterialParams.w > 0.5 ? saturate(baseAlpha) : 1.0;
}

// The material's ambient occlusion map, 1 without one
float SampleOcclusion(float2 uv)
{
    float ao = 1.0;
    if (g_UseAOMap > 0.5)
    {
        ao = g_AOMap.Sample(g_AOMap_sampler, uv).r;
    }
    return ao;
}

// The shading normal: the surface normal tilted by the material's normal map, in the tangent frame the vertex shader
// built. Without a map it is the surface normal. geoN: the interpolated vertex normal, normalized.
float3 ApplyNormalMap(PSInput In, float3 geoN)
{
    float3 N = geoN;
    if (g_UseNormalMap > 0.5)
    {
        float3x3 TBN = float3x3(normalize(In.Tangent), normalize(In.Bitangent), geoN);
        float3 tangentNormal = g_NormalMap.Sample(g_NormalMap_sampler, In.UV).xyz * 2.0 - 1.0;
        N = normalize(mul(tangentNormal, TBN));
    }
    return N;
}

// The same for a pipeline that does not trust the vertex tangents: the tangent frame comes from how the position and
// the UV change across the screen. geoN: the interpolated vertex normal, normalized.
float3 ApplyNormalMapFromDerivatives(PSInput In, float3 geoN)
{
    float3 N = geoN;
    if (g_UseNormalMap > 0.5)
    {
        float3 dp1 = ddx(In.WorldPos);
        float3 dp2 = ddy(In.WorldPos);
        float2 duv1 = ddx(In.UV);
        float2 duv2 = ddy(In.UV);

        float3 T = normalize(dp1 * duv2.y - dp2 * duv1.y);
        T = normalize(T - dot(T, N) * N);
        float3 B = cross(N, T);
        float3x3 TBN = float3x3(T, B, N);

        float3 tangentNormal = g_NormalMap.Sample(g_NormalMap_sampler, In.UV).xyz * 2.0 - 1.0;
        N = normalize(mul(tangentNormal, TBN));
    }
    return N;
}

// The angle a pixel's view ray differs from its neighbours', which tells how wide the pixel's footprint is when a
// reflection ray reads a texture (see SampleSceneTexture). Taken before any discard: a derivative needs all four pixels
// of the 2x2 block to still be running.
float ViewRaySpread(float3 worldPos, float3 cameraPos)
{
    float3 viewRay = normalize(worldPos - cameraPos);
    return max(length(ddx(viewRay)), length(ddy(viewRay)));
}

#endif // MATERIAL_SAMPLING_HLSLI
