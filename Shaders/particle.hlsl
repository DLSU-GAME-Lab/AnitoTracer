cbuffer CameraConstants : register(b0)
{
    float4x4 View;
    float4x4 Proj;
}

cbuffer ParticleConstants : register(b1)
{
    float4 CameraRight; // xyz: world-space camera right
    float4 CameraUp;    // xyz: world-space camera up
    float4 Grid;        // x: columns, y: rows
}

Texture2D g_Texture;
SamplerState g_Texture_sampler;

struct VSInput
{
    float4 PosRot : ATTRIB0;    // xyz: world position, w: rotation (radians)
    float4 SizeFrame : ATTRIB1; // xy: size, z: flipbook frame
    float4 Color : ATTRIB2;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
};

void main_vs(in VSInput VSIn, in uint VertexID : SV_VertexID, out PSInput PSIn)
{
    // Triangle strip quad: 0 (-,-) 1 (-,+) 2 (+,-) 3 (+,+)
    float2 corner = float2((VertexID >> 1) & 1, VertexID & 1) * 2.0 - 1.0;

    float s, c;
    sincos(VSIn.PosRot.w, s, c);
    float2 rotated = float2(corner.x * c - corner.y * s, corner.x * s + corner.y * c);

    float2 offset = rotated * VSIn.SizeFrame.xy * 0.5;
    float3 worldPos = VSIn.PosRot.xyz + CameraRight.xyz * offset.x + CameraUp.xyz * offset.y;

    float4 viewPos = mul(float4(worldPos, 1.0), View);
    PSIn.Pos = mul(viewPos, Proj);

    // Flipbook: frame 0 is the top-left cell, frames advance left-to-right then top-to-bottom
    float cols = max(Grid.x, 1.0);
    float rows = max(Grid.y, 1.0);
    float frame = floor(VSIn.SizeFrame.z + 0.5);
    float col = fmod(frame, cols);
    float row = floor(frame / cols);

    float2 uv = float2(corner.x * 0.5 + 0.5, 0.5 - corner.y * 0.5);
    PSIn.UV = (float2(col, row) + uv) / float2(cols, rows);
    PSIn.Color = VSIn.Color;
}

float4 main_ps(in PSInput PSIn) : SV_TARGET
{
    return g_Texture.Sample(g_Texture_sampler, PSIn.UV) * PSIn.Color;
}
