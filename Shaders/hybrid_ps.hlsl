#include "common_struct.hlsli"
#include "light_struct.hlsli"
#include "pbr_defs.hlsi"

// Bindless structures matching your C++ engine layout
struct InstanceData
{
    uint VertexOffset;
    uint IndexOffset;
    uint MaterialIndex;
    uint Padding;
};

struct BindlessMaterial
{
    int BaseColorTexIdx;
    uint Padding1;
    uint Padding2;
    uint Padding3;
};

// Global Scene Buffers
StructuredBuffer<InstanceData> g_InstanceData;
StructuredBuffer<BindlessMaterial> g_MaterialData;
StructuredBuffer<VertexInput> g_GlobalVertices;
StructuredBuffer<uint> g_GlobalIndices;

// Bindless Texture Array (Bounded to 1024 to ensure broad API compatibility)
Texture2D g_BindlessTextures[1024];
SamplerState g_LinearSampler;

// Bind the Scene Acceleration Structure for Ray Queries
RaytracingAccelerationStructure g_TLAS;

struct PSInput
{
    float4 Pos : SV_POSITION;
    centroid float3 WorldPos : TEXCOORD0;
    centroid float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float3 Tangent : TANGENT; 
    float3 Bitangent : BITANGENT; 
};

// ------------------------------------------------------------------
// Helper: Traces a shadow ray towards a light source
// ------------------------------------------------------------------
float TraceShadowRay(float3 worldPos, float3 normal, float3 lightDir, float maxDist)
{
    RayDesc ray;
    ray.Origin = worldPos + (normal * g_ShadowBias);
    ray.Direction = lightDir;
    ray.TMin = 0.001;
    ray.TMax = maxDist;

    // ACCEPT_FIRST_HIT_AND_END_SEARCH so we can evaluate non-opaque alpha
    RayQuery < RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH > q;

    // REMOVED: RAY_FLAG_FORCE_OPAQUE
    q.TraceRayInline(g_TLAS, RAY_FLAG_NONE, 0xFF, ray);

    while (q.Proceed())
    {
        if (q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
        {
            // 1. Get Hit Info
            // CustomId in your C++ TLAS build matches this InstanceID
            uint instanceID = q.CandidateInstanceID();
            uint primIndex = q.CandidatePrimitiveIndex();
            float2 bary = q.CandidateTriangleBarycentrics();

            // 2. Fetch Instance & Material
            InstanceData inst = g_InstanceData[instanceID];
            BindlessMaterial mat = g_MaterialData[inst.MaterialIndex];

            // 3. Fetch Indices
            uint baseIdx = inst.IndexOffset + (primIndex * 3);
            uint i0 = g_GlobalIndices[baseIdx + 0];
            uint i1 = g_GlobalIndices[baseIdx + 1];
            uint i2 = g_GlobalIndices[baseIdx + 2];

            // 4. Fetch Vertices (UVs only needed for alpha testing)
            float2 uv0 = g_GlobalVertices[inst.VertexOffset + i0].uv;
            float2 uv1 = g_GlobalVertices[inst.VertexOffset + i1].uv;
            float2 uv2 = g_GlobalVertices[inst.VertexOffset + i2].uv;

            // 5. Interpolate UV using Barycentrics
            float2 hitUV = uv0 + bary.x * (uv1 - uv0) + bary.y * (uv2 - uv0);

            // 6. Sample Bindless Array
            // Use SampleLevel with mip level 0, since gradients aren't available in compute/ray queries
            float alpha = 1.0;
            if (mat.BaseColorTexIdx >= 0)
            {
                alpha = g_BindlessTextures[mat.BaseColorTexIdx].SampleLevel(g_LinearSampler, hitUV, 0).a;
            }

            // 7. Alpha Cutout Test
            if (alpha >= 0.5)
            {
                q.CommitNonOpaqueTriangleHit(); // Blocks the ray
            }
        }
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
    
    //Clip transparent pixels based on alpha threshold
    if (albedo.a < 0.5)
    {
        discard;
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
    
    // Geometric normal (unperturbed) - MUST be used for shadow ray origin
    // offsetting. Using the normal-mapped shading normal instead causes the
    // ray origin to be pushed below the true geometric surface wherever the
    // bump map tilts the normal "inward" (e.g. brick centers), leading to
    // self-shadowing/acne there while edges (where the perturbed normal is
    // closer to the geometric one) stay correctly lit.
    float3 geoN = normalize(In.Normal);

    float3 N = geoN;
    float3 T = normalize(In.Tangent);
    float3 B = normalize(In.Bitangent);

    float3x3 TBN = float3x3(T, B, N);

    // --- Normal Mapping Logic (Derivative TBN) ---
    // This ensures g_NormalMap is not stripped by the compiler
    if (g_UseNormalMap > 0.5)
    {
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
            float shadowFactor = TraceShadowRay(In.WorldPos, geoN, L, 1000.0);
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
                float shadowFactor = TraceShadowRay(In.WorldPos, geoN, L, dist - 0.01);
                
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
    //float3 color = float3(1.0, 0.0, 1.0); // Debug: Magenta for missing textures)

    OutColor = float4(color, albedo.a);
}