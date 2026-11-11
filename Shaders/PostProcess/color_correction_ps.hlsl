// Display pass: the last full-screen pass of every view. Paired with Deferred/fullscreen_vs.hlsl.
// Converts the HDR scene (linear, floating point) into the 8-bit sRGB output:
//   exposure, white balance, color filter (on scene light)
//   -> tone curve
//   -> contrast, lift / gamma / gain, hue, saturation, gamma (on the display image)
//   -> dither
// The grading steps only run for views with effects, and the tone curve only for views that are tone mapped
// (see Diligent::PostProcessView); what is left then is a clamp to the display range.

#include "hdr_common.hlsli"
#include "tonemap.hlsli"

Texture2D g_SceneColor;
SamplerState g_SceneColor_sampler;

// Mirrors Diligent::ColorCorrectionConstants in src/Rendering/Pipelines/Data/PipelineDefs.hpp
cbuffer ColorCorrectionConstants
{
    float4 g_ColorFilter;   // rgb: multiplier (white balance folded in), a: exposure multiplier
    float4 g_Shadows;       // rgb: lift tint (luminance-normalized, 1 = neutral)
    float4 g_Midtones;      // rgb: gamma tint (luminance-normalized, 1 = neutral)
    float4 g_Highlights;    // rgb: gain tint (luminance-normalized, 1 = neutral)
    float g_Contrast;
    float g_Saturation;
    float g_InvGamma;
    float g_HueShift;       // Radians

    float4 g_AgXSlope;      // rgb: slope, a: offset
    float g_AgXPower;
    float g_AgXSaturation;
    float g_NeutralCompressionStart;
    float g_NeutralDesaturation;

    int g_ToneMapper;       // TONE_MAPPER_* in tonemap.hlsli
    int g_GradingEnabled;   // 0 skips everything graded after the tone curve
    float g_DitherStrength; // In 8-bit output steps; 0 = off
    float g_Padding;
};

struct FullScreenPSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

static const float MID_GREY = 0.18;

// Rotates the color around the grey axis (Rodrigues rotation about (1,1,1)/sqrt(3))
float3 RotateHue(float3 color, float angle)
{
    const float3 k = float3(0.57735, 0.57735, 0.57735);
    float cosA = cos(angle);
    float sinA = sin(angle);
    return color * cosA + cross(k, color) * sinA + k * dot(k, color) * (1.0 - cosA);
}

float3 ToneMap(float3 color)
{
    if (g_ToneMapper == TONE_MAPPER_AGX)
    {
        return ToneMapAgX(color, g_AgXSlope.rgb, g_AgXSlope.a, g_AgXPower, g_AgXSaturation);
    }
    if (g_ToneMapper == TONE_MAPPER_PBR_NEUTRAL)
    {
        return ToneMapPBRNeutral(color, g_NeutralCompressionStart, g_NeutralDesaturation);
    }
    return color; // Off: clipped at white at the end
}

// Per-pixel value in 0..1 (interleaved gradient noise)
float DitherNoise(float2 pixel)
{
    return frac(52.9829189 * frac(dot(pixel, float2(0.06711056, 0.00583715))));
}

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    float4 scene = g_SceneColor.Sample(g_SceneColor_sampler, In.UV);
    float3 color = SanitizeHDR(scene.rgb);

    // On scene light: exposure, white balance and color filter
    color *= g_ColorFilter.rgb * g_ColorFilter.a;

    // Scene light -> display image
    color = ToneMap(color);

    if (g_GradingEnabled != 0)
    {
        // Contrast in log space, pivoting around mid-grey so midtones keep their brightness
        color = MID_GREY * pow(max(color, 0.0) / MID_GREY, g_Contrast);

        // Lift / gamma / gain
        color *= g_Highlights.rgb;
        color += (g_Shadows.rgb - 1.0) * 0.1 * (1.0 - saturate(color));
        color = pow(max(color, 0.0), 1.0 / max(g_Midtones.rgb, 0.01));

        if (g_HueShift != 0.0)
            color = RotateHue(color, g_HueShift);

        float luma = dot(color, TONEMAP_LUMA_WEIGHTS);
        color = lerp(luma.xxx, color, g_Saturation);

        color = pow(max(color, 0.0), g_InvGamma);
    }

    color = saturate(color);

    // The output has 256 steps per channel. A little noise before rounding trades visible bands in smooth
    // gradients (sky, fog) for grain too fine to see. Applied in the encoded space, where the steps are even.
    if (g_DitherStrength > 0.0)
    {
        float noise = (DitherNoise(In.Pos.xy) - 0.5) * g_DitherStrength / 255.0;
        color = SRGBToLinear(saturate(LinearToSRGB(color) + noise));
    }

    return float4(color, scene.a);
}
