// outline_vs.hlsl
// Cross hatch outline pass (inverted hull): pushes the mesh outward so only its back faces show past the silhouette.
// Drawn with front-face culling, after the hatch pass.
#include "../common_struct.hlsli"
#include "crosshatch_common.hlsli"

void main_vs(in VertexInput In, out PSInput Out)
{
    float4 worldPos = mul(float4(In.Pos, 1.0), g_Model);
    float3 N = normalize(mul(In.Norm, (float3x3) g_NormalMatrix));

    // Normals give smooth meshes an even outline. Hard-surface meshes split normals at their edges, which would open
    // gaps, so they can scale away from the object's origin instead.
    float3 extrudeDir = N;
    if (g_OutlineMode > 0.5)
    {
        float3 fromOrigin = worldPos.xyz - mul(float4(0.0, 0.0, 0.0, 1.0), g_Model).xyz;
        extrudeDir = dot(fromOrigin, fromOrigin) > 1e-8 ? normalize(fromOrigin) : N;
    }

    float width = g_OutlineWidth;
    if (g_OutlineScaleWithDistance > 0.5)
    {
        width *= distance(worldPos.xyz, g_CrossHatchCameraPos.xyz);
    }
    worldPos.xyz += extrudeDir * width;

    float4 viewPos = mul(worldPos, g_View);
    Out.Pos = mul(viewPos, g_Proj);

    // Push the hull slightly back so it never z-fights the surface it surrounds
    Out.Pos.z += 0.00005 * Out.Pos.w;

    Out.WorldPos = worldPos.xyz;
    Out.Normal = N;
    Out.Tangent = normalize(mul(In.Tangent, (float3x3) g_NormalMatrix));
    Out.Bitangent = normalize(mul(In.Bitangent, (float3x3) g_NormalMatrix));
    Out.UV = In.uv;
}
