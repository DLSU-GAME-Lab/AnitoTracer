// gi_filter_ps.hlsl
// Per-pixel diffuse GI, pass 3 of 3 (DiffuseGIPass): blurs away the remaining noise. Paired with Deferred/fullscreen_vs.hlsl.
//
// A 5x5 blur whose taps are g_GIFilterStep pixels apart. Run several times with the step doubling (1, 2, 4, ...),
// it reaches far for the cost of a few small passes. A tap only counts if the guide says it lies on the same
// surface as the centre: a similar normal, and a distance that fits the centre's own slope. That keeps light from
// bleeding across edges, e.g. from a bright floor onto an object standing on it.
#include "gbuffer_common.hlsli"
#include "gi_common.hlsli"

Texture2D g_GISource;   // The light to blur: the accumulated result, or the previous blur pass
Texture2D g_GIGuide;

// Weights of a smooth 5-tap bell (1 4 6 4 1) / 16, by distance from the centre
static const float GI_KERNEL[3] = { 0.375, 0.25, 0.0625 };

float GuideDistance(int2 pixel, int2 maxPixel)
{
    return g_GIGuide.Load(int3(clamp(pixel, int2(0, 0), maxPixel), 0)).z;
}

// How fast the distance changes per pixel along one axis. Of the two neighbours, the one that changes less is used,
// so an edge on one side doesn't pass for a steep slope.
float DistanceSlope(float centre, float before, float after)
{
    float forward = after - centre;
    float backward = centre - before;
    return abs(forward) < abs(backward) ? forward : backward;
}

void main_ps(in FullScreenPSInput In, out float4 OutLight : SV_TARGET)
{
    int2 pixel = int2(In.Pos.xy);
    int2 maxPixel = int2(g_GISize.xy) - 1;

    float4 centreGuide = g_GIGuide.Load(int3(pixel, 0));
    float4 centre = g_GISource.Load(int3(pixel, 0));
    if (centreGuide.w < 0.5)
    {
        OutLight = centre; // Sky
        return;
    }

    float3 centreN = OctDecode(centreGuide.xy);
    float centreDistance = centreGuide.z;

    // The surface's slope in distance per pixel, to predict each tap's distance if it lies on the same plane
    float slopeX = DistanceSlope(centreDistance, GuideDistance(pixel - int2(1, 0), maxPixel), GuideDistance(pixel + int2(1, 0), maxPixel));
    float slopeY = DistanceSlope(centreDistance, GuideDistance(pixel - int2(0, 1), maxPixel), GuideDistance(pixel + int2(0, 1), maxPixel));

    float4 sum = centre * GI_KERNEL[0] * GI_KERNEL[0];
    float weightSum = GI_KERNEL[0] * GI_KERNEL[0];

    for (int j = -2; j <= 2; ++j)
    {
        for (int i = -2; i <= 2; ++i)
        {
            if (i == 0 && j == 0)
                continue;

            int2 offset = int2(i, j) * g_GIFilterStep;
            int2 tap = pixel + offset;
            if (any(tap < int2(0, 0)) || any(tap > maxPixel))
                continue;

            float4 tapGuide = g_GIGuide.Load(int3(tap, 0));
            if (tapGuide.w < 0.5)
                continue;

            float normalWeight = pow(saturate(dot(centreN, OctDecode(tapGuide.xy))), 32.0);

            // Distance the tap would have on the centre's plane, with room for the slope estimate being rough
            float expected = centreDistance + offset.x * slopeX + offset.y * slopeY;
            float tolerance = 0.01 * centreDistance + 0.5 * (abs(offset.x * slopeX) + abs(offset.y * slopeY)) + 0.005;
            float distanceWeight = exp(-abs(tapGuide.z - expected) / tolerance);

            float w = GI_KERNEL[abs(i)] * GI_KERNEL[abs(j)] * normalWeight * distanceWeight;
            sum += g_GISource.Load(int3(tap, 0)) * w;
            weightSum += w;
        }
    }

    OutLight = sum / weightSum;
}
