#include "../common_struct.hlsli"

cbuffer EditorConstants
{
    float4 g_WireframeColor;
};

struct GBufferOutput
{
    float4 AlbedoMetallic : SV_TARGET0;
    float4 NormalRoughness : SV_TARGET1;
    float4 WorldPos : SV_TARGET2;
};

void main_ps(in PSInput In, out GBufferOutput Out)
{
    // Write Master's custom color to the Albedo target[cite: 37]
    Out.AlbedoMetallic = float4(g_WireframeColor.rgb, 0.0);
    
    // We set Roughness (the Alpha channel) to -1.0 to flag this pixel as UNLIT!
    // Because your RGBA16_FLOAT texture supports negative values, this works perfectly.
    Out.NormalRoughness = float4(0.0, 0.0, 0.0, -1.0);
    
    // We keep WorldPos w=1.0 to ensure the lighting pass doesn't discard our wires[cite: 37]!
    Out.WorldPos = float4(In.WorldPos, 1.0);
}