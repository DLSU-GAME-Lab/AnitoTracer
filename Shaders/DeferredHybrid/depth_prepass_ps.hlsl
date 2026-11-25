// depth_prepass_ps.hlsl
// Depth-only pass for the Deferred Hybrid pipeline. Lays down the nearest depth for every pixel
// so the ray-traced shading pass only runs (and only traces rays) for visible fragments.
#include "../common_struct.hlsli"
#include "../pbr_defs.hlsli"
#include "../material_sampling.hlsli"

void main_ps(in PSInput In)
{
    // Must match the alpha test in gbuffer_ps.hlsl, or cutout holes would be filled with depth
    float alpha = SampleBaseColor(In.UV).a;

    if (!PassesAlphaTest(alpha))
    {
        discard;
    }
}
