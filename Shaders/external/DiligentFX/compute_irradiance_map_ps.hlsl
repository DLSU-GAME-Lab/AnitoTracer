// Ported from DiligentFX, Copyright 2019-2026 Diligent Graphics LLC, Apache License 2.0 (LICENSE.txt in this folder).
//   Source: Shaders/PBR/private/ComputeIrradianceMap.psh
//   Changes: reads a cubemap only (the sphere map path was dropped); always picks the mip per sample
//            (OPTIMIZE_SAMPLES); paired with Deferred/fullscreen_vs.hlsl, taking the face direction from
//            CubeFaceDirection instead of a per-face rotation matrix in a dedicated vertex shader.

// Generates an irradiance cube from an environment map using convolution.
// Each texel of the result is the sky averaged over the hemisphere around its direction, weighted by the cosine:
// the diffuse light a surface facing that way receives. SkyEnvironment draws it once per face when a sky loads.

#include "ibl_precompute_common.hlsli"
#include "../../Skybox/sky_common.hlsli"

TextureCube  g_EnvironmentMap;
SamplerState g_EnvironmentMap_sampler;

struct FullScreenPSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

float3 SampleEnvrionmentMap(float3 R, float MipLevel)
{
    return g_EnvironmentMap.SampleLevel(g_EnvironmentMap_sampler, R, MipLevel).rgb;
}

float3 IrradianceMap(float3 N)
{
    float3 T, B;
    BasisFromNormal(N, T, B);

    // When the HDRI map has points occupying a small area but with high peak brightness (such as the sun), brightness spikes appear on the irradiance map.
    // To resolve this issue, we used the approach from the comments in this post:
    // https://www.reddit.com/r/opengl/comments/l0h5fy/rendering_the_irradiance_map_for_physically_based/
    float3 Irradiance = float3(0.0, 0.0, 0.0);
    for (uint i = 0u; i < g_NumSamples; ++i)
    {
        float2 Xi = Hammersley2D(i, g_NumSamples);

        // Importance sample the hemisphere with a cosine-weighted distribution
        float3 L;
        float  pdf;
        SampleDirectionCosineHemisphere(Xi, L, pdf);
        L = normalize(L.x * T + L.y * B + L.z * N);

        // Solid angle of current smple
        float OmegaS = 1.0 / (float(g_NumSamples) * pdf);

        // Solid angle of 1 pixel across all cube faces
        float OmegaP = ComputeCubeMapPixelSolidAngle(g_EnvMapWidth, g_EnvMapHeight);

        // Applying mip bias produces better results, especially for environment maps with
        // very bright spots.
        float MipBias = 1.0;
        float MipLevel = clamp(0.5 * log2(OmegaS / max(OmegaP, 1e-10)) + MipBias, 0.0, g_EnvMipCount - 1.0);

        Irradiance += SampleEnvrionmentMap(L, MipLevel);
    }
    return Irradiance / float(g_NumSamples);
}

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    float3 N = CubeFaceDirection(g_Face, In.UV);
    return float4(IrradianceMap(N), 1.0);
}
