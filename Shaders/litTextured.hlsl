#include "common_struct.hlsli"
#include "light_struct.hlsli"

Texture2D g_Texture;
SamplerState g_Texture_sampler;

struct PSInput
{
    float4 Pos : SV_POSITION;
    float3 WorldPos : TEXCOORD0;
    float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
};

void main_vs(in VertexInput In, out PSInput Out)
{
    float4 worldPos = mul(float4(In.Pos, 1.0), g_Model);
    float4 viewPos = mul(worldPos, g_View);
    Out.Pos = mul(viewPos, g_Proj);
    
    // Convert NDC depth to [0, 1] for Vulkan
    Out.Pos.z = (Out.Pos.z + Out.Pos.w) * 0.5;
    
    Out.WorldPos = worldPos.xyz;
    
    // Transform normal to world space using the model matrix
    Out.Normal = mul(float4(In.Norm, 0.0), g_Model).xyz;
    
    Out.UV = In.uv;
}

void main_ps(in PSInput In, out float4 OutColor : SV_TARGET)
{
    float4 texColor = g_Texture.Sample(g_Texture_sampler, In.UV) * g_BaseColor;
    float3 normal = normalize(In.Normal);
    float3 viewDir = normalize(g_CameraPos.xyz - In.WorldPos);

    float3 totalDiffuse = float3(0.0, 0.0, 0.0);
    float3 totalSpecular = float3(0.0, 0.0, 0.0);

    // Directional Lights Processing
    for (int i = 0; i < g_NumDirLights; ++i)
    {
        float3 lightDir = normalize(-g_DirLights[i].Direction.xyz);
        
        // Diffuse
        float NdotL = max(dot(normal, lightDir), 0.0);
        totalDiffuse += g_DirLights[i].Color.rgb * g_DirLights[i].Color.a * NdotL;

        // Specular (Blinn-Phong)
        float3 halfDir = normalize(lightDir + viewDir);
        float NdotH = max(dot(normal, halfDir), 0.0);
        totalSpecular += g_DirLights[i].Color.rgb * pow(NdotH, 32.0) * (NdotL > 0.0 ? 1.0 : 0.0);
    }

    // Point Lights Processing
    for (int j = 0; j < g_NumPointLights; ++j)
    {
        float3 lightVec = g_PointLights[j].Position.xyz - In.WorldPos;
        float dist = length(lightVec);
        
        if (dist < g_PointLights[j].Range)
        {
            float3 lightDir = lightVec / dist;
            
            // Attenuation factor
            float attenuation = max(1.0 - (dist / g_PointLights[j].Range), 0.0);
            attenuation *= attenuation;

            // Diffuse
            float NdotL = max(dot(normal, lightDir), 0.0);
            totalDiffuse += g_PointLights[j].Color.rgb * g_PointLights[j].Color.a * NdotL * attenuation;

            // Specular
            float3 halfDir = normalize(lightDir + viewDir);
            float NdotH = max(dot(normal, halfDir), 0.0);
            totalSpecular += g_PointLights[j].Color.rgb * pow(NdotH, 32.0) * attenuation * (NdotL > 0.0 ? 1.0 : 0.0);
        }
    }

    float3 ambient = float3(0.1, 0.1, 0.1);
    float3 finalColor = (ambient + totalDiffuse) * texColor.rgb + totalSpecular;

    OutColor = float4(finalColor, texColor.a);
}