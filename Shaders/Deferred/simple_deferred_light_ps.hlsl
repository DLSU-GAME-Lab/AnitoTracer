#include "../common_struct.hlsli"
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"

// G-Buffer Inputs
Texture2D g_GBufferAlbedo;
Texture2D g_GBufferNormal;
Texture2D g_GBufferWorldPos;

SamplerState g_GBuffer_sampler;

struct FullScreenPSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};


void main_ps(in FullScreenPSInput In, out float4 OutColor : SV_TARGET)
{
    // Sample G-Buffer
    float4 albedoMetal = g_GBufferAlbedo.Sample(g_GBuffer_sampler, In.UV);
    float4 normalRough = g_GBufferNormal.Sample(g_GBuffer_sampler, In.UV);
    float4 worldPos = g_GBufferWorldPos.Sample(g_GBuffer_sampler, In.UV);
    
    // Discard background pixels
    if (worldPos.w == 0.0)
    {
        discard;
    }

    float3 albedo = albedoMetal.rgb;
    float metallic = albedoMetal.a;
    float3 N = normalize(normalRough.xyz);
    float roughness = normalRough.a;
    float3 WPos = worldPos.xyz;

    float3 V = normalize(g_CameraPos.xyz - WPos);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 Lo = float3(0.0, 0.0, 0.0);

    // Directional Lights processing (No Shadows)
    for (int i = 0; i < g_NumDirLights; ++i)
    {
        float3 L = normalize(-g_DirLights[i].Direction.xyz);
        float3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);

        if (NdotL > 0.0)
        {
            float3 radiance = g_DirLights[i].Color.rgb * g_DirLights[i].Color.a;

            float NDF = DistributionGGX(N, H, roughness);
            float G = GeometrySmith(N, V, L, roughness);
            float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

            float3 numerator = NDF * G * F;
            float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
            float3 specular = numerator / denominator;

            float3 kS = F;
            float3 kD = (float3(1.0, 1.0, 1.0) - kS) * (1.0 - metallic);

            Lo += (kD * albedo / PI + specular) * radiance * NdotL;
        }
    }

    // Ambient Lighting
    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * g_AmbientMultiplier;
    OutColor = float4(ambient + Lo, 1.0);
}