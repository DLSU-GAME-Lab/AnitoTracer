// anime_vs.hlsl
// Anime toon pass vertex shader: the standard transform, plus the smooth normal and curvature from the second stream.
#include "../common_struct.hlsli"
#include "anime_common.hlsli"

void main_vs(in AnimeVertexInput In, out AnimePSInput Out)
{
    float4 worldPos = mul(float4(In.Pos, 1.0), g_Model);
    float4 viewPos = mul(worldPos, g_View);
    Out.Pos = mul(viewPos, g_Proj);

    Out.WorldPos = worldPos.xyz;
    Out.Normal = normalize(mul(In.Norm, (float3x3) g_NormalMatrix));
    Out.Tangent = normalize(mul(In.Tangent, (float3x3) g_NormalMatrix));
    Out.Bitangent = normalize(mul(In.Bitangent, (float3x3) g_NormalMatrix));
    Out.UV = In.uv;

    Out.SmoothNormal = normalize(mul(In.SmoothData.xyz, (float3x3) g_NormalMatrix));
    Out.Curvature = In.SmoothData.w;
}
