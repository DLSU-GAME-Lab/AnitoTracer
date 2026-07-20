cbuffer CameraConstants
{
    // The combined World * View * Projection matrix
    float4x4 g_WorldViewProj;
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
};