#include "common_struct.hlsli"
#include "light_struct.hlsli"

Texture2D g_Texture;
SamplerState g_Texture_sampler;

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
    ray.Origin = worldPos + (normal * 0.015); // Increased from 0.002
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
    float4 texColor = g_Texture.Sample(g_Texture_sampler, In.UV) * g_BaseColor;
    float3 normal = normalize(In.Normal);
    float3 viewDir = normalize(g_CameraPos.xyz - In.WorldPos);

    float3 totalDiffuse = float3(0.0, 0.0, 0.0);
    float3 totalSpecular = float3(0.0, 0.0, 0.0);

    // ==================================================================
    // Directional Lights Processing
    // ==================================================================
    for (int i = 0; i < g_NumDirLights; ++i)
    {
        float3 lightDir = normalize(-g_DirLights[i].Direction.xyz);
        float NdotL = max(dot(normal, lightDir), 0.0);
        
        // Only trace a shadow ray if the surface faces the light
        float shadowFactor = 1.0;
        if (NdotL > 0.0)
        {
            // Directional lights are infinitely far away
            shadowFactor = TraceShadowRay(In.WorldPos, normal, lightDir, 1000.0);
        }

        // Apply shadow factor to both Diffuse and Specular
        totalDiffuse += g_DirLights[i].Color.rgb * g_DirLights[i].Color.a * NdotL * shadowFactor;

        // Specular (Blinn-Phong)
        float3 halfDir = normalize(lightDir + viewDir);
        float NdotH = max(dot(normal, halfDir), 0.0);
        totalSpecular += g_DirLights[i].Color.rgb * pow(NdotH, 32.0) * (NdotL > 0.0 ? 1.0 : 0.0) * shadowFactor;
    }

    // ==================================================================
    // Point Lights Processing
    // ==================================================================
    for (int j = 0; j < g_NumPointLights; ++j)
    {
        float3 lightVec = g_PointLights[j].Position.xyz - In.WorldPos;
        float dist = length(lightVec);
        
        if (dist < g_PointLights[j].Range)
        {
            float3 lightDir = lightVec / dist;
            float NdotL = max(dot(normal, lightDir), 0.0);

            // Only trace a shadow ray if the surface faces the light
            float shadowFactor = 1.0;
            if (NdotL > 0.0)
            {
                // Limit ray distance to light position (subtract small epsilon to prevent hitting point light origin)
                shadowFactor = TraceShadowRay(In.WorldPos, normal, lightDir, dist - 0.01);
            }
            
            // Attenuation factor
            float attenuation = max(1.0 - (dist / g_PointLights[j].Range), 0.0);
            attenuation *= attenuation;

            // Apply shadow factor to Diffuse and Specular
            totalDiffuse += g_PointLights[j].Color.rgb * g_PointLights[j].Color.a * NdotL * attenuation * shadowFactor;

            // Specular
            float3 halfDir = normalize(lightDir + viewDir);
            float NdotH = max(dot(normal, halfDir), 0.0);
            totalSpecular += g_PointLights[j].Color.rgb * pow(NdotH, 32.0) * attenuation * (NdotL > 0.0 ? 1.0 : 0.0) * shadowFactor;
        }
    }

    float3 ambient = float3(0.1, 0.1, 0.1);
    float3 finalColor = (ambient + totalDiffuse) * texColor.rgb + totalSpecular;

    OutColor = float4(finalColor, texColor.a);
}