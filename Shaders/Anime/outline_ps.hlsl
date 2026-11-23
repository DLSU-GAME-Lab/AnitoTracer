// outline_ps.hlsl
// Anime outline pass: flat ink, alpha tested like the toon pass so cutouts don't get a solid hull
#include "../common_struct.hlsli"
#include "../light_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../fog.hlsli"
#include "anime_common.hlsli"

void main_ps(in AnimePSInput In, out float4 OutColor : SV_TARGET)
{
    float alpha = g_BaseColorFactor.a;
    if (g_UseBaseColorMap > 0.5)
    {
        alpha *= g_BaseColorMap.Sample(g_BaseColorMap_sampler, In.UV).a;
    }

    if (!PassesAlphaTest(alpha))
    {
        discard;
    }

    OutColor = float4(ApplyFog(g_AnimeOutlineColor.rgb, In.WorldPos, In.Pos.xy), 1.0);
}
