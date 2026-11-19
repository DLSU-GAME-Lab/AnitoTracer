cbuffer CameraConstants
{
    float4x4 g_View;
    float4x4 g_Proj;
    // For motion vectors: world -> clip space this frame and last frame, both without TAA's sub-pixel jitter
    float4x4 g_ViewProjNoJitter;
    float4x4 g_PrevViewProjNoJitter;
};

cbuffer ModelConstants
{
    float4x4 g_Model;
    float4x4 g_NormalMatrix;
    float4x4 g_PrevModel;   // Last frame's g_Model
};

struct VertexInput
{
    float3 Pos : ATTRIB0;
    float3 Norm : ATTRIB1;
    float2 uv : ATTRIB2;
    float3 Tangent : ATTRIB3;
    float3 Bitangent : ATTRIB4;
};

struct PSInput
{
    float4 Pos : SV_POSITION;
    centroid float3 WorldPos : TEXCOORD0;
    centroid float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    // Clip space position without TAA jitter, this frame and last frame. Their difference on screen is the pixel's
    // motion vector (see DeferredHybrid/gbuffer_ps.hlsl); pixel shaders that don't write one ignore them.
    float4 CurrClip : TEXCOORD3;
    float4 PrevClip : TEXCOORD4;
};