// crosshatch_ps.hlsl
// Cross hatching, ported from AnitoCrossHatch: Lambert lighting, then ink strokes whose density follows the light,
// in the style picked by g_HatchMode. crosshatch_rt_ps.hlsl defines CROSSHATCH_RAYTRACED_SHADOWS before including this
// file, which traces shadows against the TLAS; without it only the key light's form shadow feeds the shadow layer.
#include "../common_struct.hlsli"
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../fog.hlsli"
#include "crosshatch_common.hlsli"

#ifdef CROSSHATCH_RAYTRACED_SHADOWS
#include "../rt_shadows.hlsli"
#endif

// Grey noise the procedural strokes are stretched from (Assets/CrossHatch/Noise)
Texture2D g_NoiseTex;
SamplerState g_NoiseTex_sampler;

// Hand-drawn stroke texture for the toon texture mode (Assets/CrossHatch/hatch_strokes.png)
Texture2D g_HatchTex;
SamplerState g_HatchTex_sampler;

// ------------------------------------------------------------------
// Helpers
// ------------------------------------------------------------------

float2 RotateUV(float2 uv, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    return float2(c * uv.x + s * uv.y, -s * uv.x + c * uv.y);
}

float Luma(float3 color)
{
    return dot(color, float3(0.299, 0.587, 0.114));
}

// Stroke pattern in 2D: noise stretched into streaks, layered (rotated and rescaled) more times the higher strength is.
// Returns ~1 on bare paper, lower on strokes. Gradients are carried through the loop explicitly, since the early
// exit makes implicit derivatives undefined.
float HatchStrokes(float2 p, float2 dpdx, float2 dpdy, float strength)
{
    static const float2 STRETCH = float2(0.025, 0.5);

    float rz = 1.0;
    for (int i = 0; i < 10; ++i)
    {
        float g = g_NoiseTex.SampleGrad(g_NoiseTex_sampler, STRETCH * p, STRETCH * dpdx, STRETCH * dpdy).r;
        g = smoothstep(-strength * 0.1, 2.3 - strength * 0.1, g);
        rz = min(1.0 - g, rz);

        p = (p.yx + 0.7) * 1.52;
        dpdx = dpdx.yx * 1.52;
        dpdy = dpdy.yx * 1.52;

        if (float(i) > strength)
            break;
    }
    return rz * 1.05;
}

// HatchStrokes projected onto the surface from the three world axes, blended by the normal
float HatchTriplanar(float3 p, float3 n, float strength, float angle)
{
    float3 dpdx = ddx(p);
    float3 dpdy = ddy(p);

    float3 v;
    v.x = HatchStrokes(RotateUV(p.yz, angle), RotateUV(dpdx.yz, angle), RotateUV(dpdy.yz, angle), strength);
    v.y = HatchStrokes(RotateUV(p.zx, angle), RotateUV(dpdx.zx, angle), RotateUV(dpdy.zx, angle), strength);
    v.z = HatchStrokes(RotateUV(p.xy, angle), RotateUV(dpdx.xy, angle), RotateUV(dpdy.xy, angle), strength);
    return dot(v, n * n);
}

// Ink coverage of the lit and detail layers (1 = ink)
float LitHatchInk(float3 worldPos, float3 N, float darkness)
{
    float eps = g_LineSmoothness;

    float lineLit = HatchTriplanar(worldPos * g_LitScale, N, darkness * g_LitDensity * g_LitThickness, g_LitAngle);
    float inkLit = 1.0 - smoothstep(darkness - eps, darkness + eps, lineLit);

    float lineDetail = HatchTriplanar(worldPos * g_DetailScale, N, g_DetailDensity * g_DetailThickness, g_DetailAngle);
    float inkDetail = 1.0 - step(0.5, lineDetail);

    return max(inkLit, inkDetail);
}

// Ink coverage of the shadow layer; keyShadow is the key light's visibility (0 = fully shadowed)
float ShadowHatchInk(float3 worldPos, float3 N, float keyShadow)
{
    float eps = g_ShadowSmoothness;
    float shadowStrength = pow(1.0 - keyShadow, g_ShadowCoreBias) * g_ShadowIntensity;

    float lineShadow = HatchTriplanar(worldPos * g_ShadowScale, N, shadowStrength * g_ShadowDensity * g_ShadowThickness, g_ShadowAngle);
    return 1.0 - smoothstep(shadowStrength - eps, shadowStrength + eps, lineShadow);
}

// ------------------------------------------------------------------
// Main
// ------------------------------------------------------------------

void main_ps(in PSInput In, out float4 OutColor : SV_TARGET)
{
    // 1. Material
    float4 albedo = g_BaseColorFactor;
    if (g_UseBaseColorMap > 0.5)
    {
        albedo *= g_BaseColorMap.Sample(g_BaseColorMap_sampler, In.UV);
    }

    // Alpha testing for foliage and cutouts
    if (albedo.a < 0.5)
    {
        discard;
    }

    float ao = 1.0;
    if (g_UseAOMap > 0.5)
    {
        ao = g_AOMap.Sample(g_AOMap_sampler, In.UV).r;
    }

    // Unperturbed normal for ray offsets and inner edge detection, so normal maps don't turn into ink noise
    float3 geoN = normalize(In.Normal);
    float3 N = geoN;
    if (g_UseNormalMap > 0.5)
    {
        float3x3 TBN = float3x3(normalize(In.Tangent), normalize(In.Bitangent), geoN);
        float3 tangentNormal = g_NormalMap.Sample(g_NormalMap_sampler, In.UV).xyz * 2.0 - 1.0;
        N = normalize(mul(tangentNormal, TBN));
    }

    float3 emissive = float3(0.0, 0.0, 0.0);
    if (g_UseEmissiveMap > 0.5)
    {
        emissive = g_EmissiveMap.Sample(g_EmissiveMap_sampler, In.UV).rgb;
    }

    // 2. Lambert lighting. lightStrength is the same sum without light colors, which drives the toon modes.
    // keyShadow is the first light's visibility, including its form shadow, and drives the shadow hatch layer.
    float3 lighting = float3(0.0, 0.0, 0.0);
    float lightStrength = 0.0;
    float keyShadow = 1.0;

#ifdef CROSSHATCH_RAYTRACED_SHADOWS
    float noise = InterleavedGradientNoise(In.Pos.xy);
#endif

    for (int i = 0; i < g_NumDirLights; ++i)
    {
        float3 L = normalize(-g_DirLights[i].Direction.xyz);
        float NdotL = max(dot(N, L), 0.0);

        float shadow = 1.0;
#ifdef CROSSHATCH_RAYTRACED_SHADOWS
        if (NdotL > 0.0)
        {
            shadow = TraceDirectionalShadow(In.WorldPos, geoN, L, noise);
        }
#endif
        if (i == 0)
        {
            keyShadow = shadow * smoothstep(0.0, 0.05, NdotL);
        }

        float contribution = g_DirLights[i].Color.a * NdotL * shadow;
        lighting += g_DirLights[i].Color.rgb * contribution;
        lightStrength += contribution;
    }

    for (int j = 0; j < g_NumPointLights; ++j)
    {
        float3 lightVec = g_PointLights[j].Position.xyz - In.WorldPos;
        float dist = length(lightVec);
        if (dist >= g_PointLights[j].Range)
            continue;

        float3 L = lightVec / dist;
        float NdotL = max(dot(N, L), 0.0);

        float attenuation = max(1.0 - (dist / g_PointLights[j].Range), 0.0);
        attenuation *= attenuation;

        float shadow = 1.0;
#ifdef CROSSHATCH_RAYTRACED_SHADOWS
        if (NdotL > 0.0)
        {
            shadow = TracePointShadow(In.WorldPos, geoN, g_PointLights[j].Position.xyz, L, noise);
        }
#endif
        if (j == 0 && g_NumDirLights == 0)
        {
            keyShadow = shadow * smoothstep(0.0, 0.05, NdotL);
        }

        float contribution = g_PointLights[j].Color.a * attenuation * NdotL * shadow;
        lighting += g_PointLights[j].Color.rgb * contribution;
        lightStrength += contribution;
    }

    float3 ambient = g_AmbientColor.rgb * g_AmbientMultiplier * ao;
    lighting += ambient;
    lightStrength += Luma(ambient);

    float3 litColor = albedo.rgb * lighting + emissive;

    // 3. Hatching
    float3 inkColor = g_InkColor.rgb;
    float3 hatchedColor = litColor;

    if (g_HatchMode == HATCH_MODE_NOISE)
    {
        float inkLit = LitHatchInk(In.WorldPos, N, 1.0 - Luma(litColor));
        float inkShadow = ShadowHatchInk(In.WorldPos, N, keyShadow);
        hatchedColor = lerp(litColor, inkColor, max(inkLit, inkShadow));
    }
    else if (g_HatchMode == HATCH_MODE_TOON_TEXTURE)
    {
        float effectiveLight = lightStrength + g_TexLightBias;

        // Two-tone cel shading of the paper, keeping the lights' tint
        float celRamp = smoothstep(0.4, 0.45, lightStrength) * keyShadow;
        float celBrightness = lerp(0.5, 1.0, celRamp);
        float3 lightTint = lighting / max(lightStrength, 0.0001);
        float tintMask = smoothstep(0.0001, 0.005, lightStrength);
        float3 paperColor = albedo.rgb * celBrightness * lerp(float3(1.0, 1.0, 1.0), lightTint, tintMask) + emissive;

        // The stroke texture sampled twice at right angles; the second direction only appears in darker areas
        float2 hatchUV = (RotateUV(In.UV - 0.5, g_TexRotation) + 0.5) * g_TexScale;
        float rawVert = g_HatchTex.Sample(g_HatchTex_sampler, hatchUV).r;
        float rawHorz = g_HatchTex.Sample(g_HatchTex_sampler, float2(hatchUV.y, -hatchUV.x)).r;

        float mapVert = saturate((1.0 - rawVert) + g_TexThickness);
        float mapHorz = saturate((1.0 - rawHorz) + g_TexThickness) * g_TexPriority;
        float strokes = max(mapVert, mapHorz);

        float ink = smoothstep(effectiveLight - g_TexSmoothness, effectiveLight + g_TexSmoothness, strokes);
        float solidInk = 1.0 - smoothstep(g_TexBlackThreshold, g_TexBlackThreshold + 0.05, effectiveLight);
        hatchedColor = lerp(paperColor, inkColor, max(ink, solidInk));

        if (g_SpecularEnabled != 0)
        {
            float specMask = smoothstep(g_SpecularThreshold, g_SpecularThreshold + 0.03, effectiveLight);
            hatchedColor = lerp(hatchedColor, float3(1.0, 1.0, 1.0), specMask);
        }
    }
    else if (g_HatchMode == HATCH_MODE_TOON_NOISE)
    {
        // Three tones: solid ink in shadow, hatched midtones, clean paper in the light
        float edgeWidth = max(g_LineSmoothness, 0.01);
        float maskLow = smoothstep(0.3 - edgeWidth, 0.3 + edgeWidth, lightStrength);
        float maskHigh = smoothstep(0.7 - edgeWidth, 0.7 + edgeWidth, lightStrength);

        float inkLit = LitHatchInk(In.WorldPos, N, 1.0 - lightStrength);
        float3 midColor = lerp(albedo.rgb * 0.7, inkColor, inkLit);

        hatchedColor = lerp(lerp(inkColor, midColor, maskLow), albedo.rgb, maskHigh) + emissive;
    }

    float3 color = lerp(litColor, hatchedColor, g_InkColor.a);

    // 4. Inner edges: ink where the normal bends sharply between neighboring pixels (creases, silhouettes inside the mesh)
    if (g_InnerEdges != 0)
    {
        float innerEdge = smoothstep(0.2, 0.4, length(fwidth(geoN)));
        color = lerp(color, inkColor, innerEdge);
    }

    color = ApplyFog(color, In.WorldPos, In.Pos.xy);
    OutColor = float4(color, albedo.a);
}
