#ifndef TONEMAP_HLSLI
#define TONEMAP_HLSLI

// Tone curves of the display pass (color_correction_ps.hlsl). Each maps HDR scene color (linear, 0 and up) to
// display color (linear, 0..1). The output target is an sRGB texture, which applies the display encoding on write.

// Values match Diligent::ToneMapper in src/Rendering/Pipelines/Data/PipelineDefs.hpp
static const int TONE_MAPPER_OFF = 0;
static const int TONE_MAPPER_AGX = 1;
static const int TONE_MAPPER_PBR_NEUTRAL = 2;

static const float3 TONEMAP_LUMA_WEIGHTS = float3(0.2126, 0.7152, 0.0722);

// ------------------------------------------------------------------
// sRGB display encoding (IEC 61966-2-1)
// ------------------------------------------------------------------
float3 SRGBToLinear(float3 srgb)
{
    float3 low = srgb / 12.92;
    float3 high = pow(max((srgb + 0.055) / 1.055, 0.0), 2.4);
    return lerp(low, high, step(float3(0.04045, 0.04045, 0.04045), srgb));
}

float3 LinearToSRGB(float3 lin)
{
    float3 low = lin * 12.92;
    float3 high = 1.055 * pow(max(lin, 0.0), 1.0 / 2.4) - 0.055;
    return lerp(low, high, step(float3(0.0031308, 0.0031308, 0.0031308), lin));
}

// ------------------------------------------------------------------
// AgX (Troy Sobotka), in the minimal form by Benjamin Wrensch:
// https://iolite-engine.com/blog_posts/minimal_agx_implementation
// Very bright and saturated light rolls off toward white the way film does, at the cost of lower contrast
// and saturation than the scene values. The look parameters put some of that back.
// ------------------------------------------------------------------

// Built from rows, for mul(matrix, color)
static const float3x3 AGX_INSET = float3x3(
    0.842479062253094,  0.0784335999999992, 0.0792237451477643,
    0.0423282422610123, 0.878468636469772,  0.0791661274605434,
    0.0423756549057051, 0.0784336,          0.879142973793104);

static const float3x3 AGX_OUTSET = float3x3(
     1.19687900512017,   -0.0980208811401368, -0.0990297440797205,
    -0.0528968517574562,  1.15190312990417,   -0.0989611768448433,
    -0.0529716355144438, -0.0980434501171241,  1.15107367264116);

// Range of scene brightness the curve covers, as log2: 10 stops under and 6.5 stops over middle grey (0.18)
static const float AGX_MIN_EV = -12.47393;
static const float AGX_MAX_EV = 4.026069;

// Polynomial fit of the AgX base contrast curve, on the 0..1 log encoded value
float3 AgXContrastApprox(float3 x)
{
    float3 x2 = x * x;
    float3 x4 = x2 * x2;

    return + 15.5     * x4 * x2
           - 40.14    * x4 * x
           + 31.96    * x4
           - 6.868    * x2 * x
           + 0.4298   * x2
           + 0.1191   * x
           - 0.00232;
}

// slope, offset, power (an ASC CDL grade) and saturation shape the look. 1, 0, 1, 1 is the plain "Base" look.
float3 ToneMapAgX(float3 color, float3 slope, float offset, float power, float saturation)
{
    // Inset: mixes a little of each channel into the others, so saturated colors fade toward white as they
    // brighten instead of each channel clipping on its own
    color = mul(AGX_INSET, max(color, 0.0));

    // Log2 encoding of the covered range, then the contrast curve
    color = clamp(log2(max(color, 1e-10)), AGX_MIN_EV, AGX_MAX_EV);
    color = (color - AGX_MIN_EV) / (AGX_MAX_EV - AGX_MIN_EV);
    color = AgXContrastApprox(color);

    // Look
    float luma = dot(color, TONEMAP_LUMA_WEIGHTS);
    color = pow(max(color * slope + offset, 0.0), power);
    color = luma + saturation * (color - luma);

    // Outset: undoes the inset's channel mixing
    color = mul(AGX_OUTSET, color);

    // The curve's result is display encoded. Linearize it, since the sRGB target encodes again on write.
    return SRGBToLinear(saturate(color));
}

// ------------------------------------------------------------------
// Khronos PBR Neutral: https://github.com/KhronosGroup/ToneMapping/tree/main/PBR_Neutral
// Leaves colors below compressionStart as authored and only rolls off the highlights above it.
// compressionStart is 0.8 and desaturation 0.15 in the Khronos specification.
// ------------------------------------------------------------------
float3 ToneMapPBRNeutral(float3 color, float compressionStart, float desaturation)
{
    // 0.04 is how much light a common non-metal reflects off its surface, on top of its base color. The curve
    // takes that much back out of the darkest channel, so base colors come out as authored instead of washed out.
    float startCompression = compressionStart - 0.04;

    color = max(color, 0.0);
    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak >= startCompression)
    {
        float d = 1.0 - startCompression;
        float newPeak = 1.0 - d * d / (peak + d - startCompression);
        color *= newPeak / peak;

        // Fade toward white as the highlight gets brighter
        float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
        color = lerp(color, float3(newPeak, newPeak, newPeak), g);
    }
    return color;
}

#endif // TONEMAP_HLSLI
