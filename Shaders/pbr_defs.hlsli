cbuffer MaterialConstants
{
    float4 g_BaseColor; // From source 1
};

cbuffer PBRMaterialConstants
{
    float4 g_BaseColorFactor;
    
    float g_MetallicFactor;
    float g_RoughnessFactor;
    float g_UseBaseColorMap;
    float g_UseMetallicRoughnessMap;
    
    float g_UseNormalMap;
    float g_UseAOMap;
    float g_UseEmissiveMap;
    float g_PaddingMat;
};

// Define a matching sampler for every texture for Vulkan compatibility
Texture2D g_BaseColorMap;
SamplerState g_BaseColorMap_sampler;

Texture2D g_MetallicRoughnessMap;
SamplerState g_MetallicRoughnessMap_sampler;

Texture2D g_NormalMap;
SamplerState g_NormalMap_sampler;

Texture2D g_AOMap;
SamplerState g_AOMap_sampler;

Texture2D g_EmissiveMap;
SamplerState g_EmissiveMap_sampler;

static const float PI = 3.14159265359;

// --- Cook-Torrance BRDF Helper Functions ---

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 0.000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / max(denom, 0.000001);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 ComputeLightRadiance(float3 N, float3 V, float3 L, float3 radiance, float3 albedo, float metallic, float roughness, float3 F0, float shadowFactor)
{
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return float3(0.0, 0.0, 0.0);

    float3 H = normalize(V + L);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    float3 specular = numerator / denominator;

    float3 kD = (float3(1.0, 1.0, 1.0) - F) * (1.0 - metallic);
    return (kD * albedo / PI + specular) * radiance * NdotL * shadowFactor;
}