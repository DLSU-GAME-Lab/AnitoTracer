#ifndef ANIME_COMMON_HLSLI
#define ANIME_COMMON_HLSLI

// Anime (cel) shading, ported from Weeb_Big2. Shared by the toon pass (anime_ps.hlsl) and the outline pass
// (outline_vs/ps.hlsl). Mirrors AnimeConstants in PipelineDefs.hpp, filled from ProjectAnimeSettings.

cbuffer AnimeConstants
{
    float4 g_AnimeBaseColor;        // rgb: tint of the lit tone (multiplies the material's color)
    float4 g_AnimeShadowColor;      // rgb: tint of the shadow tone
    float4 g_AnimeInnerEdgeColor;   // rgb: ink of the inner edges
    float4 g_AnimeOutlineColor;     // rgb: color of the outline hull
    float4 g_AnimeCameraPos;        // xyz: camera position, for the outline pass which doesn't bind LightConstants

    float g_AnimeStep;              // Light (N.L) a surface needs to be in the lit tone
    float g_AnimeInnerStep;         // Surfaces facing the camera less than this get inner edge ink (0 = off)
    float g_AnimeCurveStep;         // Surfaces bending more than this get inner edge ink (>= 1 = off)
    float g_AnimeShadowFactor;      // 0 = shadow tone is the shadow color as is, 1 = black

    float g_AnimeLightFalloff;      // 0 = flat tones, 1 = the lit tone fades toward the shadow tone with N.L (Lambert)
    float g_AnimeSpecularStrength;
    float g_AnimeBaseColorBlur;     // Mip level the base color texture is blurred to (0 = sharp)
    float g_AnimeNormalMapBlur;     // The same for the normal map

    float g_AnimeOutlineWidth;
    float g_AnimeOutlineScaleWithDistance; // 1: width is multiplied by the distance to the camera, keeping it constant on screen
    float g_AnimeOutlineSmoothNormals;     // 1: extrude along the smooth normals, which don't open gaps at hard edges
    float g_AnimePadding0;

    int g_AnimeUseSmoothNormal;     // Band the lighting with the smooth normal instead of the mesh's normal
    int g_AnimeSpecularEnabled;
    int g_AnimePadding1;
    int g_AnimePadding2;
};

// The standard vertex plus a second stream (Model::pAnimeVertexBuffer, built by BuildAnimeVertexData)
struct AnimeVertexInput
{
    float3 Pos : ATTRIB0;
    float3 Norm : ATTRIB1;
    float2 uv : ATTRIB2;
    float3 Tangent : ATTRIB3;
    float3 Bitangent : ATTRIB4;
    float4 SmoothData : ATTRIB5;    // xyz: smooth normal (welded across seams and hard edges), w: curvature 0..1
};

struct AnimePSInput
{
    float4 Pos : SV_POSITION;
    centroid float3 WorldPos : TEXCOORD0;
    centroid float3 Normal : TEXCOORD1;
    float2 UV : TEXCOORD2;
    float3 Tangent : TANGENT;
    float3 Bitangent : BITANGENT;
    float3 SmoothNormal : TEXCOORD3;
    float Curvature : TEXCOORD4;
};

#endif // ANIME_COMMON_HLSLI
