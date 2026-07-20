#include "common_struct.hlsli"

cbuffer CameraConstants
{
    // The combined World * View * Projection matrix
    float4x4 g_WorldViewProj;
};

struct PSInput
{
    float4 Pos : SV_POSITION; 
    float4 Color : COLOR0;
};

void main_vs(in VertexInput In,
          out PSInput Out)
{
    // Diligent's math library (by default) uses row-major matrices, 
    // so we multiply the vector by the matrix (Vector * Matrix).
    Out.Pos = mul(float4(In.Pos, 1.0), g_WorldViewProj);
    
    Out.Color = float4(1.0, 0.0, 0.0, 1.0);
}

void main_ps(in PSInput In,
          out float4 OutColor : SV_TARGET)
{
    // Output the interpolated color with an alpha of 1.0 (fully opaque)
    OutColor = In.Color;
}