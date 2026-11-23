// depth_prepass_ps.hlsl
// Depth-only pass for the Deferred Hybrid pipeline. Lays down the nearest depth for every pixel
// so the ray-traced shading pass only runs (and only traces rays) for visible fragments.
#include "../common_struct.hlsli"
#include "../pbr_defs.hlsli"

void main_ps(in PSInput In)
{
    // Must match the alpha test in hybrid_ps.hlsl, or cutout holes would be filled with depth
    float alpha = g_BaseColorFactor.a;
    if (g_UseBaseColorMap > 0.5)
    {
        alpha *= g_BaseColorMap.Sample(g_BaseColorMap_sampler, In.UV).a;
    }

    if (alpha < 0.5)
    {
        discard;
    }
}
