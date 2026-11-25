#include "../light_struct.hlsli"
#include "../fog.hlsli"
#include "sky_common.hlsli"
#include "skybox_io.hlsli"

// The sky cubemap built by SkyEnvironment: floating point, so an HDR sky keeps its real brightness
TextureCube g_Cubemap;
SamplerState g_Cubemap_sampler;

// Mirrors Diligent::SkyboxConstants in src/Rendering/Pipelines/Data/PipelineDefs.hpp
cbuffer SkyboxConstants
{
    float2 g_SkyboxRotation;    // (sin, cos) of the sky's rotation around the up axis
    float g_SkyboxIntensity;    // Brightness multiplier
    float g_SkyboxPadding;
};

float4 main_ps(in SkyboxPSInput In) : SV_TARGET
{
    // Sample the TextureCube using our normalized direction vector
    float3 dir = normalize(In.SampleDir);
    float3 skyColor = g_Cubemap.Sample(g_Cubemap_sampler, RotateSkyDirection(dir, g_SkyboxRotation)).rgb * g_SkyboxIntensity;

    // Scene light, not a display color: the display pass exposes and tone maps it with the rest of the frame
    skyColor = ApplySkyFog(skyColor, dir, In.Pos.xy);
    return float4(skyColor, 1.0);
}
