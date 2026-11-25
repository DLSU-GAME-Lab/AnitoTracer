#ifndef DEFERRED_HYBRID_TEMPORAL_COMMON_HLSLI
#define DEFERRED_HYBRID_TEMPORAL_COMMON_HLSLI

// What the passes that reuse last frame's results share (the per-pixel GI in gi_common.hlsli, the denoisers in
// denoise_common.hlsli): their textures are smaller than the view by a whole number along each axis, and a position
// seen last frame falls between four of their pixels.
// Needs nothing else.

// The view pixel a pixel of a smaller texture stands for: its guide, motion vector and ray start are there.
// scale: view pixels per pixel of the texture along each axis; viewportSize: the view's size in pixels.
int2 SignalToViewPixel(int2 signalPixel, int scale, int2 viewportSize)
{
    return min(signalPixel * scale, viewportSize - 1);
}

// The four pixels around a position in last frame's image, and how much each counts. A pixel only counts if last frame
// it showed the same surface, which the finder of the taps decides, so WeightSum is 0 when there is nothing to reuse.
struct BilinearTaps
{
    int2 Base;          // Top-left of the four pixels; the others are Base + (1,0), (0,1), (1,1)
    float4 Weights;     // In that order
    float WeightSum;
};

// Taps that find nothing to reuse
BilinearTaps NoTaps()
{
    BilinearTaps taps;
    taps.Base = int2(0, 0);
    taps.Weights = float4(0.0, 0.0, 0.0, 0.0);
    taps.WeightSum = 0.0;
    return taps;
}

// Where tap k (0 to 3) is, from Base
int2 BilinearTapOffset(int k)
{
    return int2(k & 1, k >> 1);
}

// How much the tap at `offset` counts for a position f (0..1 on each axis) of the way from Base to the tap after it
float BilinearTapWeight(int2 offset, float2 f)
{
    return (offset.x == 0 ? 1.0 - f.x : f.x) * (offset.y == 0 ? 1.0 - f.y : f.y);
}

// The weighted average of a texture at the taps that count (only meaningful when WeightSum > 0)
float4 LoadBilinearTaps(Texture2D history, BilinearTaps taps)
{
    float4 value = float4(0.0, 0.0, 0.0, 0.0);
    for (int k = 0; k < 4; ++k)
    {
        if (taps.Weights[k] > 0.0)
        {
            value += history.Load(int3(taps.Base + BilinearTapOffset(k), 0)) * taps.Weights[k];
        }
    }
    return value / max(taps.WeightSum, 0.0001);
}

#endif // DEFERRED_HYBRID_TEMPORAL_COMMON_HLSLI
