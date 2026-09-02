#include "../common_struct.hlsli"

struct PSInput
{
    float4 Pos : SV_POSITION;
    centroid float3 WorldPos : TEXCOORD0;
    centroid float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
};

void main_vs(in VertexInput In, out PSInput Out)
{
    float4 worldPos = mul(float4(In.Pos, 1.0), g_Model);
    float4 viewPos = mul(worldPos, g_View);
    Out.Pos = mul(viewPos, g_Proj);
    
    // Convert NDC depth to [0, 1] for Vulkan
    //Out.Pos.z = (Out.Pos.z + Out.Pos.w) * 0.5;
    
    Out.WorldPos = worldPos.xyz;
    
    // Transform normal to world space using the model matrix
    Out.Normal = normalize(mul((float3x3) g_Model, In.Norm));
    Out.Tangent = normalize(mul((float3x3) g_Model, In.Tangent));
    Out.Bitangent = normalize(mul((float3x3) g_Model, In.Bitangent));
    
    Out.UV = In.uv;
}