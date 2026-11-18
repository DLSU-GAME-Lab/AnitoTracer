#ifndef HDR_COMMON_HLSLI
#define HDR_COMMON_HLSLI

// Helpers shared by the passes that read the HDR scene color (TAA, god rays, the display pass).

// Largest value kept in the scene color. Just under what a 16-bit float channel can store (65504).
static const float HDR_MAX_VALUE = 64000.0;

// A floating point target keeps bad values that the old 8-bit targets used to hide. One NaN or infinite pixel shows up
// as a black or white dot, and spreads if it reaches a filter or the TAA history. Returns black for those, and
// clamps the rest to the range the scene format can store.
float3 SanitizeHDR(float3 color)
{
    if (any(isnan(color)) || any(isinf(color)))
    {
        return float3(0.0, 0.0, 0.0);
    }
    return clamp(color, 0.0, HDR_MAX_VALUE);
}

#endif // HDR_COMMON_HLSLI
