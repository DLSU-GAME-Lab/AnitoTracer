#ifndef SKYBOX_IO_HLSLI
#define SKYBOX_IO_HLSLI

// What the skybox vertex shader hands the pixel shader
struct SkyboxPSInput
{
    float4 Pos : SV_POSITION;
    float3 SampleDir : TEXCOORD0; // The local 3D position acts as our cubemap sampling direction
};

#endif // SKYBOX_IO_HLSLI
