// denoise_variance_ps.hlsl
// SignalDenoiser, between the temporal pass and the blur passes of a signal whose blur keeps its light
// (denoise_filter_conserve_ps.hlsl). Paired with Deferred/fullscreen_vs.hlsl.
//
// Blurs the variance the temporal pass measured a little: it is itself a noisy estimate, and keeps single pixels from
// standing out. denoise_filter_ps.hlsl does that inside every blur pass, but only for the pixel it is blurring. The
// conserving blur also needs the tolerance of each neighbour that takes from that pixel, and both sides must read the
// same one, so the blurred variance is made once here and read as it is.
#include "denoise_common.hlsli"

Texture2D g_DnVariance; // From the temporal pass

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    int2 pixel = int2(In.Pos.xy);
    int2 maxPixel = int2(g_DnSize.xy) - 1;

    float4 variance = float4(0.0, 0.0, 0.0, 0.0);
    for (int j = -1; j <= 1; ++j)
    {
        for (int i = -1; i <= 1; ++i)
        {
            float w = (i == 0 ? 0.5 : 0.25) * (j == 0 ? 0.5 : 0.25);
            variance += g_DnVariance.Load(int3(clamp(pixel + int2(i, j), int2(0, 0), maxPixel), 0)) * w;
        }
    }
    return variance;
}
