#include "common_struct.hlsli"
#include "light_struct.hlsli"
#include "pbr_defs.hlsli"
#include "material_sampling.hlsli"
#include "fog.hlsli"
#include "light_shading.hlsli"

void main_ps(in PSInput In, out float4 OutColor : SV_TARGET)
{
    // 1. Material Sampling
    float4 albedo = SampleBaseColor(In.UV);

    float metallic;
    float roughness;
    SampleMetallicRoughness(In.UV, metallic, roughness);

    float ao = SampleOcclusion(In.UV);

    // The tangent frame comes from the screen space derivatives
    float3 N = ApplyNormalMapFromDerivatives(In, normalize(In.Normal));

    float3 V = normalize(g_CameraPos.xyz - In.WorldPos);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, metallic);
    float3 Lo = float3(0.0, 0.0, 0.0);

    // Constant shadow factor (1.0 = fully lit) since RT is disabled
    const float shadowFactor = 1.0;

    // 2. Directional Lights
    for (int i = 0; i < g_NumDirLights; ++i)
    {
        float3 L = normalize(-g_DirLights[i].Direction.xyz);
        if (dot(N, L) <= 0.0)
            continue;

        float3 radiance = g_DirLights[i].Color.rgb * g_DirLights[i].Color.a;
        Lo += ShadeRoundLight(N, V, L, tan(g_DirLights[i].Direction.w), radiance, albedo.rgb, metallic, roughness, F0, shadowFactor, float3(1.0, 1.0, 1.0));
    }

    // 3. Point, spot and rectangular lights
    for (int j = 0; j < g_NumPointLights; ++j)
    {
        LocalLightSample light;
        if (!EvaluateLocalLight(j, In.WorldPos, N, V, light))
            continue;

        Lo += ShadeLocalLight(N, V, light, albedo.rgb, metallic, roughness, F0, shadowFactor, float3(1.0, 1.0, 1.0));
    }

    // --- Emissive Mapping Logic ---
    float3 emissive = SampleEmissive(In.UV);

    // 4. Ambient & Emissive Combined
    float3 ambient = g_AmbientColor.rgb * albedo.rgb * ao * g_AmbientMultiplier;
    float3 color = ambient + Lo + emissive;
    color = ApplyFog(color, In.WorldPos, In.Pos.xy);

    OutColor = float4(color, albedo.a);
}