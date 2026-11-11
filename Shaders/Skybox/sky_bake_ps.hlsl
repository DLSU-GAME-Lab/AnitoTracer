// Draws one face of the sky cubemap from the sky's source image. Paired with Deferred/fullscreen_vs.hlsl.
// SkyEnvironment runs it six times when a sky loads, so everything that reads the sky afterwards (the skybox,
// the lighting cubes filtered from it) gets a floating point cubemap, whatever the source file was.
//
// The source is a cubemap by default. sky_bake_panorama_ps.hlsl defines SKY_SOURCE_PANORAMA and includes
// this file, for sources that are a 360 degree panorama.

#include "sky_common.hlsli"

#ifdef SKY_SOURCE_PANORAMA
Texture2D g_Source;
#else
TextureCube g_Source;
#endif
SamplerState g_Source_sampler;

// Mirrors SkyBakeConstants in src/Rendering/Pipelines/Utils/SkyEnvironment.cpp
cbuffer SkyBakeConstants
{
    uint g_Face;        // Cubemap face being drawn, see CubeFaceDirection
    float g_SourceLod;  // Source mip to read, above 0 when the source has more detail than the cubemap keeps
    float2 g_Padding;
};

struct FullScreenPSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

// Largest value kept. Just under what a 16-bit float channel can store, so a very bright sun doesn't become infinite.
static const float SKY_MAX_VALUE = 64000.0;

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    float3 dir = CubeFaceDirection(g_Face, In.UV);

#ifdef SKY_SOURCE_PANORAMA
    float3 color = g_Source.SampleLevel(g_Source_sampler, DirectionToPanoramaUV(dir), g_SourceLod).rgb;
#else
    float3 color = g_Source.SampleLevel(g_Source_sampler, dir, g_SourceLod).rgb;
#endif

    if (any(isnan(color)) || any(isinf(color)))
    {
        color = float3(0.0, 0.0, 0.0);
    }
    return float4(clamp(color, 0.0, SKY_MAX_VALUE), 1.0);
}
