// denoise_filter_ps.hlsl
// SignalDenoiser, pass 2 of 2: blurs away the noise that averaging over time left. Paired with Deferred/fullscreen_vs.hlsl.
//
// A 5x5 blur whose taps are g_DnFilterStep pixels apart. Run several times with the step doubling (1, 2, 4, ...),
// it reaches far for the cost of a few small passes. A tap only counts if:
//   - the guide says it lies on the same surface as the centre: a similar normal, and a position close to the
//     centre's plane. That keeps light from bleeding across edges, e.g. from a bright floor onto an object on it.
//   - its value differs from the centre's by no more than the signal's own uncertainty there (the variance measured
//     by the temporal pass) explains. Where the rays agree, e.g. inside a shadow or on a mirror, nothing is blurred
//     and edges in the signal stay sharp; where they disagree, e.g. in a penumbra or on a new pixel, the blur is wide.
//   - for reflections, it lies within the patch of screen the surface's highlight lobe covers: wide for a rough
//     surface reflecting something far away, a point for a mirror.
#include "denoise_common.hlsli"

Texture2D g_DnSource;   // The signal to blur: the temporal pass's result, or the previous blur pass
Texture2D g_DnVariance; // From the temporal pass, see there

// Weights of a smooth 5-tap bell (1 4 6 4 1) / 16, by distance from the centre
static const float KERNEL[3] = { 0.375, 0.25, 0.0625 };

float4 main_ps(in FullScreenPSInput In) : SV_TARGET
{
    int2 pixel = int2(In.Pos.xy);
    int2 maxPixel = int2(g_DnSize.xy) - 1;

    float4 centreGuide = DnLoadGuide(g_DnGuide, pixel);
    float4 centre = g_DnSource.Load(int3(pixel, 0));
    if (DnIsEmpty(centreGuide))
    {
        return centre;
    }

    // How far a tap's value may be from the centre's and still count
    bool compareValues = g_DnBrightnessSigma > 0.0;
    float4 tolerance = float4(1.0, 1.0, 1.0, 1.0);
    float centreBrightness = DnBrightness(centre);
    if (compareValues)
    {
        // The variance is itself a noisy estimate; a small blur over it keeps single pixels from standing out
        float4 variance = float4(0.0, 0.0, 0.0, 0.0);
        for (int vj = -1; vj <= 1; ++vj)
        {
            for (int vi = -1; vi <= 1; ++vi)
            {
                float w = (vi == 0 ? 0.5 : 0.25) * (vj == 0 ? 0.5 : 0.25);
                variance += g_DnVariance.Load(int3(clamp(pixel + int2(vi, vj), int2(0, 0), maxPixel), 0)) * w;
            }
        }

        float4 deviation = sqrt(max(variance, 0.0)) * g_DnBrightnessSigma * g_DnUncertaintyScale;
        if (g_DnKind == SIGNAL_VISIBILITY)
        {
            // Every ray around here agrees (fully lit or fully shadowed, as most pixels are): nothing to blur
            if (max(max(variance.x, variance.y), max(variance.z, variance.w)) <= 0.0)
            {
                return centre;
            }
            tolerance = deviation + 0.01;
        }
        else
        {
            float t = deviation.x + 0.01 * centreBrightness + 0.001;
            tolerance = float4(t, t, t, t);
        }
    }

    // Reflections: the blur's radius in pixels, from how wide the highlight lobe is where it meets what is
    // reflected, as seen from the camera (the reflected image appears that much farther away than the surface)
    bool reflection = g_DnKind == SIGNAL_REFLECTION;
    float lobeFalloff = 0.0;
    if (reflection)
    {
        float lobeAngle = 0.5 * centreGuide.w * centreGuide.w;
        float hitDistance = max(centre.a, 0.0);
        float pixelSize = max((centreGuide.z + hitDistance) * g_DnPixelAngle * float(g_DnScale), 0.000001);
        float lobeRadius = min(lobeAngle * hitDistance / pixelSize, 32.0);
        if (lobeRadius < 0.3)
        {
            return centre; // As good as a mirror
        }
        lobeFalloff = 0.5 / (lobeRadius * lobeRadius);
    }

    float3 centreN = OctDecode(centreGuide.xy);
    float centreDistance = centreGuide.z;

    // Positions relative to the camera, from the guide's distance along each pixel's view direction
    float2 centreView = float2(DnViewPixel(pixel)) + 0.5;
    float3 direction = DnViewDirection(centreView);
    float3 directionX = DnViewDirection(centreView + float2(float(g_DnScale), 0.0)) - direction;
    float3 directionY = DnViewDirection(centreView + float2(0.0, float(g_DnScale))) - direction;
    float3 centrePos = normalize(direction) * centreDistance;
    // How far off the centre's plane a tap may lie. Well above the precision of the stored distance (about
    // 0.05%), which would otherwise show up as rings on flat walls.
    float planeTolerance = 0.01 * centreDistance + 0.01;

    float centreWeight = KERNEL[0] * KERNEL[0];
    float4 sum = centre * centreWeight;
    float4 weightSum = float4(centreWeight, centreWeight, centreWeight, centreWeight);

    for (int j = -2; j <= 2; ++j)
    {
        for (int i = -2; i <= 2; ++i)
        {
            if (i == 0 && j == 0)
                continue;

            // A tap past the border reads the pixel at the border: dropping it instead would take a whole row of
            // taps away at once at fixed distances from the border, which shows as bands
            int2 tap = clamp(pixel + int2(i, j) * g_DnFilterStep, int2(0, 0), maxPixel);
            int2 offset = tap - pixel;
            if (offset.x == 0 && offset.y == 0)
                continue;

            float4 tapGuide = DnLoadGuide(g_DnGuide, tap);
            if (DnIsEmpty(tapGuide))
                continue;

            float normalWeight = pow(saturate(dot(centreN, OctDecode(tapGuide.xy))), 32.0);

            // Full weight up to half the tolerance off the centre's plane, none beyond it
            float3 tapPos = normalize(direction + float(offset.x) * directionX + float(offset.y) * directionY) * tapGuide.z;
            float planeDistance = abs(dot(centreN, tapPos - centrePos));
            float distanceWeight = saturate(2.0 - 2.0 * planeDistance / planeTolerance);

            float w = KERNEL[abs(i)] * KERNEL[abs(j)] * normalWeight * distanceWeight;
            if (reflection)
            {
                // A surface of another roughness reflects a differently blurred image
                w *= saturate(1.0 - 8.0 * abs(tapGuide.w - centreGuide.w));
                w *= exp(-float(dot(offset, offset)) * lobeFalloff);
            }
            if (w <= 0.0)
                continue;

            float4 value = g_DnSource.Load(int3(tap, 0));
            float4 weights = float4(w, w, w, w);
            if (compareValues)
            {
                if (g_DnKind == SIGNAL_VISIBILITY)
                {
                    weights *= exp(-abs(value - centre) / tolerance);
                }
                else
                {
                    weights *= exp(-abs(DnBrightness(value) - centreBrightness) / tolerance.x);
                }
            }

            sum += value * weights;
            weightSum += weights;
        }
    }

    return sum / weightSum;
}
