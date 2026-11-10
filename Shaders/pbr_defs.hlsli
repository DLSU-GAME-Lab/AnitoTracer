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
    float g_OverrideFlags; // Bit 1: roughness overridden, bit 2: metallic overridden

    float4 g_EmissiveFactor; // rgb: emissive color * strength (HDR)
};

// Applies the metallic/roughness texture, skipping channels overridden by the model component
void ApplyMetallicRoughnessMap(float4 mrSample, inout float metallic, inout float roughness)
{
    uint flags = (uint)g_OverrideFlags;
    if ((flags & 1u) == 0u) roughness *= mrSample.g;
    if ((flags & 2u) == 0u) metallic *= mrSample.b;
}

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

// Light emitted by the surface: the material's emissive color * strength, masked by the emissive map when there is one
float3 SampleEmissive(float2 uv)
{
    float3 emissive = g_EmissiveFactor.rgb;
    if (g_UseEmissiveMap > 0.5)
    {
        emissive *= g_EmissiveMap.Sample(g_EmissiveMap_sampler, uv).rgb;
    }
    return emissive;
}

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

// Split-sum environment BRDF, analytic fit (Karis, "Physically Based Shading on Mobile")
// Returns (scale, bias) so that the specular response is F0 * x + y
float2 EnvBRDFApprox(float NdotV, float roughness)
{
    const float4 c0 = float4(-1.0, -0.0275, -0.572, 0.022);
    const float4 c1 = float4(1.0, 0.0425, 1.04, -0.04);
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    return float2(-1.04, 1.04) * a004 + r.zw;
}

// specularScale lets callers apply multi-scattering energy compensation to the specular lobe
float3 ComputeLightRadiance(float3 N, float3 V, float3 L, float3 radiance, float3 albedo, float metallic, float roughness, float3 F0, float shadowFactor, float3 specularScale)
{
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return float3(0.0, 0.0, 0.0);

    float3 H = normalize(V + L);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    float3 specular = numerator / denominator * specularScale;

    float3 kD = (float3(1.0, 1.0, 1.0) - F) * (1.0 - metallic);
    return (kD * albedo / PI + specular) * radiance * NdotL * shadowFactor;
}

float3 ComputeLightRadiance(float3 N, float3 V, float3 L, float3 radiance, float3 albedo, float metallic, float roughness, float3 F0, float shadowFactor)
{
    return ComputeLightRadiance(N, V, L, radiance, albedo, metallic, roughness, F0, shadowFactor, float3(1.0, 1.0, 1.0));
}