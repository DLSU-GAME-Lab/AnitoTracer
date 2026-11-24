#ifndef SCREEN_COMMON_HLSLI
#define SCREEN_COMMON_HLSLI

// Screen space helpers shared by the full-screen passes (post-processing, the Deferred and Deferred Hybrid lighting,
// the denoisers, TAA): the vertex shader's output, and moving between UV, clip space and world space.
// Needs nothing else, so it can be included first.

// What Deferred/fullscreen_vs.hlsl gives every full-screen pass
struct FullScreenPSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

// Where a clip space position lands on screen: UV in 0..1, (0, 0) at the top left
float2 ClipToUV(float4 clip)
{
    float2 ndc = clip.xy / clip.w;
    return float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
}

// The clip space position of a screen position (UV in 0..1, (0, 0) at the top left) at a depth (0 near .. 1 far)
float4 UVToClip(float2 uv, float depth)
{
    return float4(uv.x * 2.0 - 1.0, 1.0 - 2.0 * uv.y, depth, 1.0);
}

// invViewProj takes clip space to world space (the matrix the position was drawn with)
float3 ClipToWorld(float4 clip, float4x4 invViewProj)
{
    float4 world = mul(clip, invViewProj);
    return world.xyz / world.w;
}

// A depth buffer loses precision with distance, so a position rebuilt from depth can sit off the real surface, by tenths
// of a unit a few hundred units out. Two depth steps, mapped back to world units, is how far it can be off. clip is the
// position's clip space position, worldPos what ClipToWorld made of it.
float DepthPositionError(float4 clip, float3 worldPos, float4x4 invViewProj)
{
    float depthStep = asfloat(asuint(clip.z) + 2) - clip.z;
    float4 worldNext = mul(float4(clip.xy, clip.z + depthStep, 1.0), invViewProj);
    return length(worldNext.xyz / worldNext.w - worldPos);
}

#endif // SCREEN_COMMON_HLSLI
