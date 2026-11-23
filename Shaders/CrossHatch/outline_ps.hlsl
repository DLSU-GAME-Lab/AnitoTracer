// outline_ps.hlsl
// Cross hatch outline pass: flat ink, alpha tested like the hatch pass so cutouts don't get a solid hull
#include "../common_struct.hlsli"
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../fog.hlsli"
#include "crosshatch_common.hlsli"

void main_ps(in PSInput In, out float4 OutColor : SV_TARGET)
{
    float alpha = g_BaseColorFactor.a;
    if (g_UseBaseColorMap > 0.5)
    {
        alpha *= g_BaseColorMap.Sample(g_BaseColorMap_sampler, In.UV).a;
    }

    if (alpha < 0.5)
    {
        discard;
    }

    OutColor = float4(ApplyFog(g_InkColor.rgb, In.WorldPos, In.Pos.xy), 1.0);
}
