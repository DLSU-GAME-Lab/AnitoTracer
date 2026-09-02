cbuffer CameraConstants
{
    float4x4 g_View;
    float4x4 g_Proj;
};

cbuffer ModelConstants
{
    float4x4 g_Model;
};

struct VertexInput
{
    float3 Pos : ATTRIB0;
    float3 Norm : ATTRIB1;
    float2 uv : ATTRIB2;
    float3 Tangent : ATTRIB3;
    float3 Bitangent : ATTRIB4;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    centroid float3 WorldPos : TEXCOORD0;
    centroid float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
};