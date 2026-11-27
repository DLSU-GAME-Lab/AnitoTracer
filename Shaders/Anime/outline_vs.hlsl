// outline_vs.hlsl
// Anime outline pass (inverted hull): pushes the mesh outward so only its back faces show past the silhouette.
// Drawn with front-face culling, after the toon pass.
#include "../common_struct.hlsli"
#include "anime_common.hlsli"

void main_vs(in AnimeVertexInput In, out AnimePSInput Out)
{
    float4 worldPos = mul(float4(In.Pos, 1.0), g_Model);
    float3 N = normalize(mul(In.Norm, (float3x3) g_NormalMatrix));

    // The mesh's own normals are split at hard edges, which would open gaps in the hull; the smooth normals are not
    float3 extrudeDir = N;
    if (g_AnimeOutlineSmoothNormals > 0.5)
    {
        float3 smoothN = mul(In.SmoothData.xyz, (float3x3) g_NormalMatrix);
        extrudeDir = dot(smoothN, smoothN) > 1e-8 ? normalize(smoothN) : N;
    }

    float width = g_AnimeOutlineWidth;
    if (g_AnimeOutlineScaleWithDistance > 0.5)
    {
        width *= distance(worldPos.xyz, g_AnimeCameraPos.xyz);
    }
    worldPos.xyz += extrudeDir * width;

    float4 viewPos = mul(worldPos, g_View);
    Out.Pos = mul(viewPos, g_Proj);

    // Push the hull slightly back so it never z-fights the surface it surrounds
    Out.Pos.z += 0.00005 * Out.Pos.w;

    Out.WorldPos = worldPos.xyz;
    Out.Normal = N;
    Out.Tangent = float3(0.0, 0.0, 0.0);
    Out.Bitangent = float3(0.0, 0.0, 0.0);
    Out.UV = In.uv;
    Out.SmoothNormal = extrudeDir;
    Out.Curvature = 0.0;
}
