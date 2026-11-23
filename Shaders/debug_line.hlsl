cbuffer CameraConstants : register(b0)
{
    float4x4 View;
    float4x4 Proj;
}

struct VSInput
{
    float3 Pos : ATTRIB0;
    float4 Color : ATTRIB1;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};

void main_vs(in VSInput VSIn, out PSInput PSIn)
{
    float4 worldPos = float4(VSIn.Pos, 1.0);
    float4 viewPos = mul(worldPos, View);
    PSIn.Pos = mul(viewPos, Proj);
    PSIn.Color = VSIn.Color;
}

float4 main_ps(in PSInput PSIn) : SV_TARGET
{
    return PSIn.Color;
}