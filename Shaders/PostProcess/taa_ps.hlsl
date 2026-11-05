// Temporal anti-aliasing resolve. Paired with Deferred/fullscreen_vs.hlsl.
// The scene was drawn with a sub-pixel jittered projection. Each pixel is reprojected into last frame's accumulated history
// using the scene depth, the history is clipped to this frame's local color range so stale colors don't ghost, and the two are blended.
// Writes the anti-aliased image for the next stage (target 0) and the new history (target 1). Colors are linear (sRGB targets).

Texture2D g_CurrentColor;   // This frame, same size as the view
Texture2D g_HistoryColor;   // Accumulated result of last frame, same size as the view
SamplerState g_HistoryColor_sampler;
Texture2D g_SceneDepth;     // Read by pixel, so it may be larger than the view (only its top-left corner is used)

// Mirrors Diligent::TemporalAAConstants in src/Rendering/Pipelines/Data/PipelineDefs.hpp
cbuffer TemporalAAConstants
{
    float4x4 g_InvViewProj;     // Jittered clip -> world
    float4x4 g_ViewProj;        // Unjittered world -> clip
    float4x4 g_PrevViewProj;    // Last frame's unjittered world -> clip
    float4 g_ViewportSize;      // xy: size in pixels, zw: 1 / size
    float g_HistoryBlend;
    float g_Sharpness;
    float g_HistoryValid;
    float g_HasDepth;
};

struct FullScreenPSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

struct TAAOutput
{
    float4 Color   : SV_TARGET0;
    float4 History : SV_TARGET1;
};

// Clipping in YCoCg fits the neighborhood's colors much tighter than RGB, since luma and chroma vary independently
float3 RGBToYCoCg(float3 c)
{
    return float3(dot(c, float3(0.25, 0.5, 0.25)), dot(c, float3(0.5, 0.0, -0.5)), dot(c, float3(-0.25, 0.5, -0.25)));
}

float3 YCoCgToRGB(float3 c)
{
    return float3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z);
}

float3 LoadCurrent(int2 pixel)
{
    return g_CurrentColor.Load(int3(clamp(pixel, int2(0, 0), int2(g_ViewportSize.xy) - 1), 0)).rgb;
}

float LoadDepth(int2 pixel)
{
    if (g_HasDepth == 0.0)
        return 1.0;
    return g_SceneDepth.Load(int3(clamp(pixel, int2(0, 0), int2(g_ViewportSize.xy) - 1), 0)).r;
}

float2 ClipToUV(float4 clip)
{
    float2 ndc = clip.xy / clip.w;
    return float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
}

float3 SampleHistory(float2 uv)
{
    return g_HistoryColor.SampleLevel(g_HistoryColor_sampler, uv, 0.0).rgb;
}

// Catmull-Rom filtered history in 5 bilinear taps (the 4 corners of the 4x4 footprint are dropped).
// A plain bilinear fetch would blur the history a little more every frame.
float3 SampleHistoryCatmullRom(float2 uv)
{
    float2 samplePos = uv * g_ViewportSize.xy;
    float2 texPos1 = floor(samplePos - 0.5) + 0.5;
    float2 f = samplePos - texPos1;

    float2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    float2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    float2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    float2 w3 = f * f * (-0.5 + 0.5 * f);

    float2 w12 = w1 + w2;
    float2 texPos0 = (texPos1 - 1.0) * g_ViewportSize.zw;
    float2 texPos3 = (texPos1 + 2.0) * g_ViewportSize.zw;
    float2 texPos12 = (texPos1 + w2 / w12) * g_ViewportSize.zw;

    float3 result = SampleHistory(float2(texPos12.x, texPos0.y)) * (w12.x * w0.y)
                  + SampleHistory(float2(texPos0.x, texPos12.y)) * (w0.x * w12.y)
                  + SampleHistory(texPos12) * (w12.x * w12.y)
                  + SampleHistory(float2(texPos3.x, texPos12.y)) * (w3.x * w12.y)
                  + SampleHistory(float2(texPos12.x, texPos3.y)) * (w12.x * w3.y);
    float weight = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;

    return max(result / weight, 0.0);
}

// Pulls color toward the box center until it lies inside the box
float3 ClipToBox(float3 color, float3 boxMin, float3 boxMax)
{
    float3 center = 0.5 * (boxMax + boxMin);
    float3 extents = 0.5 * (boxMax - boxMin) + 1e-4;
    float3 offset = color - center;
    float3 unit = abs(offset / extents);
    float maxUnit = max(unit.x, max(unit.y, unit.z));
    return maxUnit > 1.0 ? center + offset / maxUnit : color;
}

TAAOutput main_ps(in FullScreenPSInput In)
{
    int2 pixel = int2(In.Pos.xy);
    float2 uv = In.Pos.xy * g_ViewportSize.zw;

    float4 currentSample = g_CurrentColor.Load(int3(pixel, 0));
    float3 current = currentSample.rgb;

    // 3x3 neighborhood statistics, and the closest depth so silhouettes reproject with the foreground's motion
    float3 m1 = 0.0;
    float3 m2 = 0.0;
    float3 minColor = 1e10;
    float3 maxColor = -1e10;
    float closestDepth = 1.0;
    int2 closestOffset = int2(0, 0);

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            int2 offset = int2(x, y);
            float3 c = RGBToYCoCg(LoadCurrent(pixel + offset));
            m1 += c;
            m2 += c * c;
            minColor = min(minColor, c);
            maxColor = max(maxColor, c);

            float d = LoadDepth(pixel + offset);
            if (d < closestDepth)
            {
                closestDepth = d;
                closestOffset = offset;
            }
        }
    }

    // Variance clipping: a box around the mean, no larger than the neighborhood's actual range
    float3 mean = m1 / 9.0;
    float3 sigma = sqrt(max(m2 / 9.0 - mean * mean, 0.0));
    float3 boxMin = max(minColor, mean - 1.25 * sigma);
    float3 boxMax = min(maxColor, mean + 1.25 * sigma);

    // Reproject: rebuild the world position under the (jittered) sample, then compare where it lands now and last frame without jitter.
    // Using the difference rather than the previous position directly removes the jitter, so a still camera reads the history in place.
    float2 depthUV = (float2(pixel + closestOffset) + 0.5) * g_ViewportSize.zw;
    float4 clipPos = float4(depthUV.x * 2.0 - 1.0, 1.0 - 2.0 * depthUV.y, closestDepth, 1.0);
    float4 worldPos = mul(clipPos, g_InvViewProj);
    worldPos /= worldPos.w;

    float4 currClip = mul(float4(worldPos.xyz, 1.0), g_ViewProj);
    float4 prevClip = mul(float4(worldPos.xyz, 1.0), g_PrevViewProj);
    float2 historyUV = uv + (ClipToUV(prevClip) - ClipToUV(currClip));

    bool historyValid = g_HistoryValid > 0.0 && prevClip.w > 0.0 && currClip.w > 0.0
        && all(historyUV >= 0.0) && all(historyUV <= 1.0);

    float3 resolved = current;
    if (historyValid)
    {
        float3 historyYCoCg = ClipToBox(RGBToYCoCg(SampleHistoryCatmullRom(historyUV)), boxMin, boxMax);
        float3 currentYCoCg = RGBToYCoCg(current);

        // Weighting each by inverse luma keeps single bright pixels from flickering through the blend
        float historyWeight = g_HistoryBlend / (1.0 + historyYCoCg.x);
        float currentWeight = (1.0 - g_HistoryBlend) / (1.0 + currentYCoCg.x);
        resolved = YCoCgToRGB((historyYCoCg * historyWeight + currentYCoCg * currentWeight) / (historyWeight + currentWeight));
    }
    resolved = max(resolved, 0.0);
    if (any(isnan(resolved)))
        resolved = current;

    // Unsharp mask against the neighborhood's mean. Only the displayed image is sharpened, so it never accumulates in the history.
    float3 display = max(resolved + g_Sharpness * (resolved - YCoCgToRGB(mean)), 0.0);

    TAAOutput Out;
    Out.Color = float4(display, currentSample.a);
    Out.History = float4(resolved, 1.0);
    return Out;
}
