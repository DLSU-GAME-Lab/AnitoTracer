// Ported from DiligentFX, Copyright 2019-2026 Diligent Graphics LLC, Apache License 2.0 (LICENSE.txt in this folder).
//   Source: Shaders/PBR/private/PrefilterEnvMap.psh
//   Changes: reads a cubemap only (the sphere map path was dropped); always picks the mip per sample
//            (OPTIMIZE_SAMPLES); paired with Deferred/fullscreen_vs.hlsl, taking the face direction from
//            CubeFaceDirection instead of a per-face rotation matrix in a dedicated vertex shader.

// Blurs an environment map the way a surface of one roughness reflects it (GGX), for one mip of the specular cube:
// mip 0 is a mirror, the last mip the roughest surface. SkyEnvironment draws every face of every mip when a sky loads.

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

// https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
float3 PrefilterEnvMap( float PerceptualRoughness, float3 R )
{
    // https://bruop.github.io/ibl/
    // Even though all papers describe split-sum approximation as
    //
    //      [(1/N) Sum_i{ L(li) }] * [(1/N) Sum_i{ f(v, li)  * (n, li) / pdf(v, li) }]
    //
    // What is actually computed instead of the first sum is
    //
    //      (4 / Sum_i{ (n, li) } * Sum_i{ (n, li) * L(li) * (v,h) / (D(h) * (n,h)) }
    //
    // Karis doesn't provide any mathematic justification for the additional summation in the denominator, or why we should evaluate
    // Li importance sampling GGX. These empirical terms seem to provide the best correction for our split sum approximation for a constant Li.

    float3 N = R;
    float3 V = R;
    float3 PrefilteredColor = float3(0.0, 0.0, 0.0);
    float TotalWeight = 0.0;
    for( uint i = 0u; i < g_NumSamples; i++ )
    {
        float2 Xi = Hammersley2D( i, g_NumSamples );
        float3 H  = ImportanceSampleGGX( Xi, PerceptualRoughness, N );
        float3 L  = 2.0 * dot(V, H) * H - V;
        float NoL = clamp(dot(N, L), 0.0, 1.0);
        float VoH = clamp(dot(V, H), 0.0, 1.0);
        if(NoL > 0.0 && VoH > 0.0)
        {
            // https://placeholderart.wordpress.com/2015/07/28/implementation-notes-runtime-environment-map-filtering-for-image-based-lighting/

            // Probability Distribution Function
            float AlphaRoughness = PerceptualRoughness * PerceptualRoughness;
            float pdf = max(SmithGGXSampleDirectionPDF(V, N, L, AlphaRoughness), 0.0001);

            // Solid angle of current smple
            float OmegaS = 1.0 / (float(g_NumSamples) * pdf);

            // Solid angle of 1 pixel across all cube faces
            float OmegaP = ComputeCubeMapPixelSolidAngle(g_EnvMapWidth, g_EnvMapHeight);

            // Applying mip bias produces better results, especially for environment maps with
            // very bright spots.
            float MipBias = 1.0;
            float MipLevel = (AlphaRoughness == 0.0) ? 0.0 : clamp(0.5 * log2(OmegaS / max(OmegaP, 1e-10)) + MipBias, 0.0, g_EnvMipCount - 1.0);

            PrefilteredColor += SampleEnvrionmentMap(L, MipLevel) * NoL;
            TotalWeight += NoL; // Sum_i{ (n, li) }
        }
    }
    return PrefilteredColor / TotalWeight;
}

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    float3 R = CubeFaceDirection(g_Face, In.UV);
    return float4(PrefilterEnvMap(g_Roughness, R), 1.0);
}
