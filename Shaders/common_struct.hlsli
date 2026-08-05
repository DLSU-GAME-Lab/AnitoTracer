cbuffer CameraConstants
{
    // The separate View and Projection matrices
    float4x4 g_View;
    float4x4 g_Proj;
};

// New constant buffer for model properties
cbuffer ModelConstants
{
    float4x4 g_Model;
};

cbuffer MaterialConstants
{
    float4 g_BaseColor;
};

struct VertexInput
{
    float3 Pos : ATTRIB0;
    float3 Norm : ATTRIB1;
    float2 uv : ATTRIB2;
    float3 Tangent : ATTRIB3; 
    float3 Bitangent : ATTRIB4; 
};