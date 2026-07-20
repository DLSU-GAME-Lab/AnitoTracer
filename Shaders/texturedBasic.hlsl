#include "common_struct.hlsli"

// Texture and Sampler definition
Texture2D g_Texture;
SamplerState g_Texture_sampler;

struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD;
};

void main_vs(in VertexInput In, out PSInput Out)
{
    // Apply matrix transformation[cite: 3]
    Out.Pos = mul(float4(In.Pos, 1.0), g_WorldViewProj);
    
    Out.Pos.z = (Out.Pos.z + Out.Pos.w) * 0.5;
    
    // Pass the UV coordinates from the vertex input[cite: 4]
    Out.UV = In.uv;
}

void main_ps(in PSInput In, out float4 OutColor : SV_TARGET)
{
    // Sample the texture using the UV coordinates
    OutColor = g_Texture.Sample(g_Texture_sampler, In.UV) * g_BaseColor;
}