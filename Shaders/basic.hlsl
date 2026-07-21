#include "common_struct.hlsli"

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
    
    // 1. Transform to World Space
    float4 worldPos = mul(float4(In.Pos, 1.0), g_Model);
    
    // 2. Transform to View Space
    float4 viewPos = mul(worldPos, g_View);
    
    // 3. Transform to Clip Space
    Out.Pos = mul(viewPos, g_Proj);
    
    Out.Color = float4(1.0, 0.0, 0.0, 1.0);
}

void main_ps(in PSInput In,
          out float4 OutColor : SV_TARGET)
{
    // Output the interpolated color with an alpha of 1.0 (fully opaque)
    OutColor = In.Color;
}