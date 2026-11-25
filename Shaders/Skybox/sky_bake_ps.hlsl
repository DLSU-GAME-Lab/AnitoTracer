// Draws one face of the sky cubemap from the sky's source image. Paired with Deferred/fullscreen_vs.hlsl.
// SkyEnvironment runs it six times when a sky loads, so everything that reads the sky afterwards (the skybox,
// the lighting cubes filtered from it) gets a floating point cubemap, whatever the source file was.
//
// The source is a cubemap by default. sky_bake_panorama_ps.hlsl defines SKY_SOURCE_PANORAMA and includes
// this file, for sources that are a 360 degree panorama.

#include "sky_common.hlsli"
#include "../Common/hdr_common.hlsli"
#include "../Common/screen_common.hlsli"

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

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    float3 dir = CubeFaceDirection(g_Face, In.UV);

#ifdef SKY_SOURCE_PANORAMA
    float3 color = g_Source.SampleLevel(g_Source_sampler, DirectionToPanoramaUV(dir), g_SourceLod).rgb;
#else
    float3 color = g_Source.SampleLevel(g_Source_sampler, dir, g_SourceLod).rgb;
#endif

    return float4(SanitizeHDR(color), 1.0);
}
