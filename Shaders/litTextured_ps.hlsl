#include "common_struct.hlsli"
#include "light_struct.hlsli"
#include "pbr_defs.hlsi"

// Bind the Scene Acceleration Structure for Ray Queries
RaytracingAccelerationStructure g_TLAS;

struct PSInput
{
    float4 Pos : SV_POSITION;
    centroid float3 WorldPos : TEXCOORD0;
    centroid float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
};

// ------------------------------------------------------------------
// Helper: Traces a shadow ray towards a light source
// ------------------------------------------------------------------
float TraceShadowRay(float3 worldPos, float3 normal, float3 lightDir, float maxDist)
{
    RayDesc ray;
    ray.Origin = worldPos + (normal * g_ShadowBias); // Increased from 0.002
    ray.Direction = lightDir;
    ray.TMin = 0.001;
    ray.TMax = maxDist;

    RayQuery < RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER > q;

    // Force opaque so the rayquery doesn't wait for any-hit shader confirmation
    q.TraceRayInline(g_TLAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);

    // The traversal state machine requires a loop to walk through the BVH
    while (q.Proceed())
    {
    }

    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        return 0.0;
    }

    return 1.0;
}

void main_ps(in PSInput In, out float4 OutColor : SV_TARGET)
{
    // 1. Material Sampling using individual matched samplers
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
        roughness *= mrSample.g;
        metallic *= mrSample.b;
    }
    roughness = max(roughness, 0.05);

    float ao = 1.0;
    if (g_UseAOMap > 0.5)
    {
        ao = g_AOMap.Sample(g_AOMap_sampler, In.UV).r;
    }

    float3 N = normalize(In.Normal);
    
    // --- Normal Mapping Logic (Derivative TBN) ---
    // This ensures g_NormalMap is not stripped by the compiler
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

    float3 V = normalize(g_CameraPos.xyz - In.WorldPos);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, metallic);
    float3 Lo = float3(0.0, 0.0, 0.0);

    // 2. Directional Lights
    for (int i = 0; i < g_NumDirLights; ++i)
    {
        float3 L = normalize(-g_DirLights[i].Direction.xyz);
        float3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);

        if (NdotL > 0.0)
        {
            float shadowFactor = TraceShadowRay(In.WorldPos, N, L, 1000.0);
            float3 radiance = g_DirLights[i].Color.rgb * g_DirLights[i].Color.a;

            // Cook-Torrance BRDF
            float NDF = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, L, roughness);
            float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

            float3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
            float3 specular = numerator / denominator;

            float3 kS = F;
            float3 kD = (float3(1.0, 1.0, 1.0) - kS) * (1.0 - metallic);

            Lo += (kD * albedo.rgb / PI + specular) * radiance * NdotL * shadowFactor;
        }
    }

    // 3. Point Lights
    for (int j = 0; j < g_NumPointLights; ++j)
    {
        float3 lightVec = g_PointLights[j].Position.xyz - In.WorldPos;
        float dist = length(lightVec);

        if (dist < g_PointLights[j].Range)
        {
            float3 L = lightVec / dist;
            float3 H = normalize(V + L);
            float NdotL = max(dot(N, L), 0.0);

            if (NdotL > 0.0)
            {
                float shadowFactor = TraceShadowRay(In.WorldPos, N, L, dist - 0.01);
                
                float attenuation = max(1.0 - (dist / g_PointLights[j].Range), 0.0);
                attenuation *= attenuation;
                float3 radiance = g_PointLights[j].Color.rgb * g_PointLights[j].Color.a * attenuation;

                float NDF = DistributionGGX(N, H, roughness);
                float G = GeometrySmith(N, V, L, roughness);
                float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

                float3 numerator = NDF * G * F;
                float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
                float3 specular = numerator / denominator;

                float3 kS = F;
                float3 kD = (float3(1.0, 1.0, 1.0) - kS) * (1.0 - metallic);

                Lo += (kD * albedo.rgb / PI + specular) * radiance * NdotL * shadowFactor;
            }
        }
    }

    // --- Emissive Mapping Logic ---
    // This ensures g_EmissiveMap is not stripped by the compiler
    float3 emissive = float3(0.0, 0.0, 0.0);
    if (g_UseEmissiveMap > 0.5)
    {
        emissive = g_EmissiveMap.Sample(g_EmissiveMap_sampler, In.UV).rgb;
    }

    // 4. Ambient & Emissive Combined
    float3 ambient = float3(0.03, 0.03, 0.03) * albedo.rgb * ao * g_AmbientMultiplier;
    float3 color = ambient + Lo + emissive;

    OutColor = float4(color, albedo.a);
}