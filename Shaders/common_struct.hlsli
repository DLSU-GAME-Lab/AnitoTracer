struct VSInput
{
    float3 Pos : ATTRIBUTE0;
    float4 Color : ATTRIBUTE1;
};

struct VertexInput
{
    float3 Pos : ATTRIBUTE0;
    float3 Norm : ATTRIBUTE1;
    float2 uv : ATTRIBUTE2;
};