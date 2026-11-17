// Ported from DiligentFX, Copyright 2019-2026 Diligent Graphics LLC, Apache License 2.0 (LICENSE.txt in this folder).
//   Source: Shaders/PBR/private/PBR_PrecomputeCommon.fxh, Shaders/Common/public/PBR_Common.fxh,
//           Shaders/Common/public/ShaderUtilities.fxh
//   Changes: only the functions the two sky cube filters need are kept, gathered into this one file;
//            the sphere map helpers were dropped (the filters here always read a cubemap); the filters' shared
//            constants moved here, with a face index in place of the per-face rotation matrix.

#ifndef IBL_PRECOMPUTE_COMMON_HLSLI
#define IBL_PRECOMPUTE_COMMON_HLSLI

// Sampling helpers for compute_irradiance_map_ps.hlsl and prefilter_env_map_ps.hlsl.

#ifndef PI
#   define PI 3.1415926536
#endif

// Constants of both filters. Mirrors IBLPrecomputeConstants in src/Rendering/Pipelines/Utils/SkyEnvironment.cpp
cbuffer IBLPrecomputeConstants
{
    float g_Roughness;      // Prefilter only: the roughness this mip is filtered for
    float g_EnvMapWidth;    // Face size of the cubemap being filtered
    float g_EnvMapHeight;
    float g_EnvMipCount;

    uint  g_NumSamples;
    uint  g_Face;           // Cubemap face being drawn, see CubeFaceDirection in Skybox/sky_common.hlsli
    uint  g_Padding0;
    uint  g_Padding1;
};

float2 Hammersley2D(uint i, uint N)
{
    // Radical inverse based on http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
    uint bits = reversebits(i);
    float rdi = float(bits) * 2.3283064365386963e-10;
    return float2(float(i) / float(N), rdi);
}

void BasisFromNormal(in  float3 N,
                     out float3 T,
                     out float3 B)
{
    T = normalize(cross(N, abs(N.y) > 0.5 ? float3(1.0, 0.0, 0.0) : float3(0.0, 1.0, 0.0)));
    B = cross(T, N);
}

void SampleDirectionCosineHemisphere(in  float2 UV,  // Normal random variables
                                     out float3 Dir, // Direction
                                     out float  Prob // Probability of the generated direction
                                     )
{
    Dir.x = cos(2.0 * PI * UV.x) * sqrt(1.0 - UV.y);
    Dir.y = sin(2.0 * PI * UV.x) * sqrt(1.0 - UV.y);
    Dir.z = sqrt(UV.y);

    // Avoid zero probability
    Prob = max(Dir.z, 1e-6) / PI;
}

// Based on http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
float3 ImportanceSampleGGX(float2 Xi, float PerceptualRoughness, float3 N)
{
    float AlphaRoughness = PerceptualRoughness * PerceptualRoughness;
    float a2             = AlphaRoughness * AlphaRoughness;

    float Phi      = 2.0 * PI * Xi.x;
    float CosTheta = sqrt( saturate((1.0 - Xi.y) / (1.0 + (a2 - 1.0) * Xi.y)) );
    float SinTheta = sqrt( saturate(1.0 - CosTheta * CosTheta) );
    float3 H;
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;
    float3 UpVector = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 TangentX = normalize( cross( UpVector, N ) );
    float3 TangentY = cross( N, TangentX );
    // Tangent to world space
    return TangentX * H.x + TangentY * H.y + N * H.z;
}

// Solid angle of 1 pixel across all cube faces
float ComputeCubeMapPixelSolidAngle(float Width, float Height)
{
    return 4.0 * PI / (6.0 * Width * Height);
}

float NormalDistribution_GGX(float NdotH, float AlphaRoughness)
{
    // "Sampling the GGX Distribution of Visible Normals" (2018) by Eric Heitz - eq. (1)
    // https://jcgt.org/published/0007/04/01/

    // Make sure we reasonably handle AlphaRoughness == 0
    // (which corresponds to delta function)
    AlphaRoughness = max(AlphaRoughness, 1e-3);

    float a2  = AlphaRoughness * AlphaRoughness;
    float nh2 = NdotH * NdotH;
    float f   = nh2 * a2 + (1.0 - nh2);
    return a2 / max(PI * f * f, 1e-9);
}

float SmithGGXMasking(float NdotV, float AlphaRoughness)
{
    float a2 = AlphaRoughness * AlphaRoughness;

    // G1(V) of "Sampling the GGX Distribution of Visible Normals", eq. (2), for an isotropic roughness
    float Denom = NdotV + sqrt(a2 + (1.0 - a2) * NdotV * NdotV);
    return 2.0 * max(NdotV, 0.0) / max(Denom, 1e-6);
}

float SmithGGXSampleDirectionPDF(float3 V, float3 N, float3 L, float AlphaRoughness)
{
    // Micronormal is the halfway vector
    float3 H = normalize(V + L);

    float NdotH = dot(H, N);
    float NdotV = dot(N, V);
    float NdotL = dot(N, L);
    //float VdotH = dot(V, H);
    if (NdotH > 0.0 && NdotV > 0.0 && NdotL > 0.0)
    {
        // Note that [1] uses notation N for the micronormal, but in our case N is the macronormal,
        // while micronormal is H (aka the halfway vector).
        float NDF = NormalDistribution_GGX(NdotH, AlphaRoughness); // (1) - D(N)
        float G1  = SmithGGXMasking(NdotV, AlphaRoughness);        // (2) - G1(V)

        float VNDF = G1 /* * VdotH */ * NDF / NdotV; // (3) - Dv(N)
        return  VNDF / (4.0 /* * VdotH */); // (17) - VdotH cancels out
    }
    else
    {
        return 0.0;
    }
}

#endif // IBL_PRECOMPUTE_COMMON_HLSLI
