// denoise_filter_conserve_ps.hlsl
// SignalDenoiser, pass 2 of 2 for a signal whose blur must not lose light (SIGNAL_LIGHT with Settings::PreserveLight):
// the same 5x5 edge-stopping blur as denoise_filter_ps.hlsl, shaped as an exchange between pixels instead of an average.
// Paired with Deferred/fullscreen_vs.hlsl.
//
// denoise_filter_ps.hlsl gives each pixel the average of the neighbours it accepts, divided by the weights it kept. That
// is not light-conserving. A bright pixel among dark ones averages itself away, and the dark ones, which accept nothing
// that bright, take none of what it gave up. With a few bright rays among many dark ones (a small emitter seen by a few
// rays per pixel) that loses light, and since the first blur's result is the history the next frame accumulates onto, the
// loss comes back every frame: a small glowing object lit its surroundings with about half the light it should.
//
// Here a pair of neighbouring pixels exchanges light: each gives the other the same share of its value, so what one
// gains is exactly what the other loses. The share is the kernel's weight, turned down by the guide and by how far apart
// the two values are, measured against the less certain of the two pixels: the one whose value is the noisier says how
// much difference is noise. (Taking only the pixel's own tolerance would leave a bright pixel among dark ones alone, as
// the dark ones, sure of themselves, would take nothing from it.) For both sides to see the same share:
//   - the kernel and the guide's weights are symmetric: the same normal weight, and the smaller of the two plane tests
//     (each pixel's plane tests the other's position);
//   - a pixel's tolerance is read from the same blurred variance (denoise_variance_ps.hlsl) wherever it is needed.
// Where every tap is accepted this is the plain 5x5 blur, like the other. Where taps are turned down, a pixel keeps their
// share of its own value, instead of the accepted taps getting it, so edges in the signal are blurred a little less.
#include "denoise_common.hlsli"

Texture2D g_DnSource;   // The signal to blur: the temporal pass's result, or the previous blur pass
Texture2D g_DnVariance; // The variance, blurred by denoise_variance_ps.hlsl

// Weights of a smooth 5-tap bell (1 4 6 4 1) / 16, by distance from the centre
static const float KERNEL[3] = { 0.375, 0.25, 0.0625 };

// How far two pixels' values may be apart for the noise to explain it: the pixel's own uncertainty, as in denoise_filter_ps.hlsl
float Tolerance(float variance, float brightness)
{
    return sqrt(max(variance, 0.0)) * g_DnBrightnessSigma * g_DnUncertaintyScale + 0.01 * brightness + 0.001;
}

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

    bool compareValues = g_DnBrightnessSigma > 0.0;
    float centreBrightness = Luminance(centre.rgb);
    float centreTolerance = Tolerance(g_DnVariance.Load(int3(pixel, 0)).x, centreBrightness);

    float3 centreN = OctDecode(centreGuide.xy);
    float centreDistance = centreGuide.z;

    // Positions relative to the camera, from the guide's distance along each pixel's view direction
    float2 centreView = float2(DnViewPixel(pixel)) + 0.5;
    float3 direction = DnViewDirection(centreView);
    float3 directionX = DnViewDirection(centreView + float2(float(g_DnScale), 0.0)) - direction;
    float3 directionY = DnViewDirection(centreView + float2(0.0, float(g_DnScale))) - direction;
    float3 centrePos = normalize(direction) * centreDistance;
    // How far off a pixel's plane a neighbour may lie (see denoise_filter_ps.hlsl)
    float centrePlaneTolerance = 0.01 * centreDistance + 0.01;

    float4 taken = float4(0.0, 0.0, 0.0, 0.0);  // What the pixel takes from its neighbours
    float given = 0.0;                          // The share of its own value it gives them (the same shares, in turn)

    for (int j = -2; j <= 2; ++j)
    {
        for (int i = -2; i <= 2; ++i)
        {
            if (i == 0 && j == 0)
                continue;

            // A tap past the border reads the pixel at the border (see denoise_filter_ps.hlsl)
            int2 tap = clamp(pixel + int2(i, j) * g_DnFilterStep, int2(0, 0), maxPixel);
            int2 offset = tap - pixel;
            if (offset.x == 0 && offset.y == 0)
                continue;

            float4 tapGuide = DnLoadGuide(g_DnGuide, tap);
            if (DnIsEmpty(tapGuide))
                continue;

            float3 tapN = OctDecode(tapGuide.xy);
            float normalWeight = pow(saturate(dot(centreN, tapN)), 32.0);

            // Full weight up to half the tolerance off the plane, none beyond it: for each pixel's plane, and the smaller
            // of the two counts, so the weight is the same seen from either pixel
            float3 tapPos = normalize(direction + float(offset.x) * directionX + float(offset.y) * directionY) * tapGuide.z;
            float3 apart = tapPos - centrePos;
            float fromCentre = saturate(2.0 - 2.0 * abs(dot(centreN, apart)) / centrePlaneTolerance);
            float fromTap = saturate(2.0 - 2.0 * abs(dot(tapN, apart)) / (0.01 * tapGuide.z + 0.01));

            float w = KERNEL[abs(i)] * KERNEL[abs(j)] * normalWeight * min(fromCentre, fromTap);
            if (w <= 0.0)
                continue;

            float4 value = g_DnSource.Load(int3(tap, 0));
            if (compareValues)
            {
                float brightness = Luminance(value.rgb);
                float tapTolerance = Tolerance(g_DnVariance.Load(int3(tap, 0)).x, brightness);
                w *= exp(-abs(brightness - centreBrightness) / max(centreTolerance, tapTolerance));
            }

            taken += value * w;
            given += w;
        }
    }

    // What is left of the pixel's own value (never below the kernel's centre: the weights of the taps add up to 1 - 0.375^2)
    return centre * (1.0 - given) + taken;
}
