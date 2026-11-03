// Full-screen color grading pass. Paired with Deferred/fullscreen_vs.hlsl.
// The source is the game view's color buffer; since it's an sRGB target, samples come back in linear space.

Texture2D g_SceneColor;
SamplerState g_SceneColor_sampler;

// Mirrors Diligent::ColorCorrectionConstants in src/Rendering/Pipelines/PipelineDefs.hpp
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
};

struct FullScreenPSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

static const float3 LUMA_WEIGHTS = float3(0.2126, 0.7152, 0.0722);
static const float MID_GREY = 0.18;

// Rotates the color around the grey axis (Rodrigues rotation about (1,1,1)/sqrt(3))
float3 RotateHue(float3 color, float angle)
{
    const float3 k = float3(0.57735, 0.57735, 0.57735);
    float cosA = cos(angle);
    float sinA = sin(angle);
    return color * cosA + cross(k, color) * sinA + k * dot(k, color) * (1.0 - cosA);
}

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    float4 scene = g_SceneColor.Sample(g_SceneColor_sampler, In.UV);
    float3 color = scene.rgb;

    // Exposure, white balance and color filter
    color *= g_ColorFilter.rgb * g_ColorFilter.a;

    // Contrast in log space, pivoting around mid-grey so midtones keep their brightness
    color = MID_GREY * pow(max(color, 0.0) / MID_GREY, g_Contrast);

    // Lift / gamma / gain
    color *= g_Highlights.rgb;
    color += (g_Shadows.rgb - 1.0) * 0.1 * (1.0 - saturate(color));
    color = pow(max(color, 0.0), 1.0 / max(g_Midtones.rgb, 0.01));

    if (g_HueShift != 0.0)
        color = RotateHue(color, g_HueShift);

    float luma = dot(color, LUMA_WEIGHTS);
    color = lerp(luma.xxx, color, g_Saturation);

    color = pow(max(color, 0.0), g_InvGamma);

    return float4(saturate(color), scene.a);
}
