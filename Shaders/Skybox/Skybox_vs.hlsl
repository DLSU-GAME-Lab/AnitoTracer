#include "../common_struct.hlsli"

struct SkyboxPSInput
{
	float4 Pos : SV_POSITION;
	float3 SampleDir : TEXCOORD0; // The local 3D position acts as our cubemap sampling direction
};

// Procedural cube vertices (36 vertices for 12 triangles)
// Generated dynamically to spare you from needing a Vertex Buffer! ✨
static const float3 CUBE_VERTICES[36] =
{
    // Front face (-Z)
	float3(-1.0, -1.0, -1.0), float3(-1.0, 1.0, -1.0), float3(1.0, 1.0, -1.0),
    float3(-1.0, -1.0, -1.0), float3(1.0, 1.0, -1.0), float3(1.0, -1.0, -1.0),
    // Back face (+Z)
    float3(1.0, -1.0, 1.0), float3(1.0, 1.0, 1.0), float3(-1.0, 1.0, 1.0),
    float3(1.0, -1.0, 1.0), float3(-1.0, 1.0, 1.0), float3(-1.0, -1.0, 1.0),
    // Left face (-X)
    float3(-1.0, -1.0, 1.0), float3(-1.0, 1.0, 1.0), float3(-1.0, 1.0, -1.0),
    float3(-1.0, -1.0, 1.0), float3(-1.0, 1.0, -1.0), float3(-1.0, -1.0, -1.0),
    // Right face (+X)
    float3(1.0, -1.0, -1.0), float3(1.0, 1.0, -1.0), float3(1.0, 1.0, 1.0),
    float3(1.0, -1.0, -1.0), float3(1.0, 1.0, 1.0), float3(1.0, -1.0, 1.0),
    // Top face (+Y)
    float3(-1.0, 1.0, -1.0), float3(-1.0, 1.0, 1.0), float3(1.0, 1.0, 1.0),
    float3(-1.0, 1.0, -1.0), float3(1.0, 1.0, 1.0), float3(1.0, 1.0, -1.0),
    // Bottom face (-Y)
    float3(-1.0, -1.0, 1.0), float3(-1.0, -1.0, -1.0), float3(1.0, -1.0, -1.0),
    float3(-1.0, -1.0, 1.0), float3(1.0, -1.0, -1.0), float3(1.0, -1.0, 1.0)
};

void main_vs(in uint VertId : SV_VertexID, out SkyboxPSInput Out)
{
	float3 pos = CUBE_VERTICES[VertId];
    
    // Pass the raw local position directly to the pixel shader. 
    // For a cube centered at the origin, the local position IS the directional vector!
	Out.SampleDir = pos;

    // 1. Strip the translation from the View matrix
    // Because C++ transposes the GLM matrix before sending (so it matches HLSL row-major layout),
    // the translation data lives in the 4th row (index 3). We just zero it out!
	float4x4 viewRotOnly = g_View;
	viewRotOnly[3].xyz = float3(0.0, 0.0, 0.0);

    // Apply rotation and projection
	float4 viewPos = mul(float4(pos, 1.0), viewRotOnly);
	Out.Pos = mul(viewPos, g_Proj);

    // 2. Force the depth to exactly 1.0 (the far clipping plane)
    // After perspective divide (z/w), setting z = w ensures depth is 1.0.
	Out.Pos.z = Out.Pos.w;
}