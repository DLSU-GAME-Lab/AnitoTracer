// denoise_temporal_ps.hlsl
// SignalDenoiser, pass 1 of 2: averages each pixel's rays over time. Paired with Deferred/fullscreen_vs.hlsl.
//
// Each pixel finds where its surface was on screen last frame (the G-buffer's motion vectors say, for moving
// objects too) and blends this frame's ray into the value stored there. The more frames a pixel has gathered, the
// less each new ray changes it, down to g_DnMinBlend, so the image settles while still following changes.
//
// Last frame's value is only used where it shows the same surface: its guide must have a matching distance and
// normal. Where it doesn't (something moved out of the way, the camera turned to a new area) the pixel starts over.
// Where the surface is the same but what it shows has changed (a shadow moved across it), the value is pulled into
// the range this frame's rays around the pixel cover, so it catches up at once instead of leaving a trail.
//
// It also measures how uncertain the result still is (its variance), which tells the blur pass how hard to blur:
// widely where rays disagree or the pixel is new, not at all where they agree.
#include "denoise_common.hlsli"

Texture2D g_DnSignal;       // This frame's rays
Texture2D g_DnPrevGuide;    // Last frame's guide
Texture2D g_DnVelocity;     // The G-buffer's motion vectors: xy, in UV units, read by view pixel
Texture2D g_DnPrevSignal;   // Last frame's output of this pass after its first blur
Texture2D g_DnPrevMoments;  // Last frame's Moments output

struct TemporalOutput
{
    float4 Signal : SV_TARGET0;     // Same layout as the rays, averaged over time
    float4 Moments : SV_TARGET1;    // x: frames this pixel has gathered, y: mean brightness of its rays, z: mean of the squared brightness
    float4 Variance : SV_TARGET2;   // Variance still left in Signal: per channel for SIGNAL_VISIBILITY, of the brightness otherwise
};

static const float MAX_HISTORY_LENGTH = 255.0;
// Until a pixel has gathered this many frames, its own rays are too few to tell how much they disagree
static const float SETTLED_HISTORY_LENGTH = 16.0;
// Reflections on surfaces rougher than this are too blurred for the reflected image's own motion to matter
static const float VIRTUAL_MOTION_MAX_ROUGHNESS = 0.35;

// Brightness as the moments keep it: capped, so that its square still fits the half floats they are stored in
static const float MAX_MOMENT_BRIGHTNESS = 200.0;

float MomentBrightness(float4 value)
{
    return clamp(Luminance(value.rgb), -MAX_MOMENT_BRIGHTNESS, MAX_MOMENT_BRIGHTNESS);
}

// Finds the four signal pixels around a position in last frame's image, and how much each counts: a pixel only counts
// if last frame it showed the same surface, so WeightSum is 0 when there is nothing to reuse.
// prevUV: where to look in last frame's image.
// N, roughness: the surface now. distanceA, distanceB: how far from the camera it may have been then; a pixel is
// accepted when last frame's distance there is within `tolerance` (a fraction) of either.
BilinearTaps FindHistory(float2 prevUV, float3 N, float roughness, float distanceA, float distanceB, float tolerance)
{
    BilinearTaps taps = NoTaps();

    if (any(prevUV < 0.0) || any(prevUV > 1.0))
    {
        return taps;
    }

    // Position among last frame's signal pixels, whose data sits at the centre of view pixel p * g_DnScale
    float2 samplePos = (prevUV * g_DnViewportSize.xy - 0.5) / float(g_DnScale);
    taps.Base = int2(floor(samplePos));
    float2 f = samplePos - float2(taps.Base);
    int2 maxPixel = int2(g_DnSize.xy) - 1;

    for (int k = 0; k < 4; ++k)
    {
        int2 offset = BilinearTapOffset(k);
        int2 tap = taps.Base + offset;
        if (any(tap < int2(0, 0)) || any(tap > maxPixel))
            continue;

        float4 prevGuide = DnLoadGuide(g_DnPrevGuide, tap);
        if (prevGuide.w <= 0.0)
            continue;
        float offA = abs(prevGuide.z - distanceA) - (tolerance * distanceA + 0.02);
        float offB = abs(prevGuide.z - distanceB) - (tolerance * distanceB + 0.02);
        if (min(offA, offB) > 0.0)
            continue;
        if (dot(OctDecode(prevGuide.xy), N) < 0.9)
            continue;
        if (g_DnKind == SIGNAL_REFLECTION && abs(prevGuide.w - roughness) > 0.1)
            continue;

        float w = BilinearTapWeight(offset, f);
        taps.Weights[k] = w;
        taps.WeightSum += w;
    }
    return taps;
}

void main_ps(in FullScreenPSInput In, out TemporalOutput Out)
{
    int2 pixel = int2(In.Pos.xy);
    int2 maxPixel = int2(g_DnSize.xy) - 1;

    float4 current = ZeroNonFinite(g_DnSignal.Load(int3(pixel, 0)));
    Out.Signal = current;
    Out.Moments = float4(0.0, 0.0, 0.0, 0.0);
    Out.Variance = float4(0.0, 0.0, 0.0, 0.0);

    float4 guide = DnLoadGuide(g_DnGuide, pixel);
    if (DnIsEmpty(guide))
    {
        return;
    }

    // What this frame's rays say around the pixel: their mean and spread over 5x5, and their range over 3x3.
    // Neighbours on other surfaces are included: they only widen the range, which errs on the side of keeping history.
    float4 sum = float4(0.0, 0.0, 0.0, 0.0);
    float4 sumSquares = float4(0.0, 0.0, 0.0, 0.0);
    float brightnessSum = 0.0;
    float brightnessSquares = 0.0;
    float4 nearMin = current;
    float4 nearMax = current;
    for (int j = -2; j <= 2; ++j)
    {
        for (int i = -2; i <= 2; ++i)
        {
            float4 value = ZeroNonFinite(g_DnSignal.Load(int3(clamp(pixel + int2(i, j), int2(0, 0), maxPixel), 0)));
            sum += value;
            sumSquares += value * value;

            float brightness = MomentBrightness(value);
            brightnessSum += brightness;
            brightnessSquares += brightness * brightness;

            if (abs(i) <= 1 && abs(j) <= 1)
            {
                nearMin = min(nearMin, value);
                nearMax = max(nearMax, value);
            }
        }
    }
    float4 mean = sum / 25.0;
    float4 deviation = sqrt(max(sumSquares / 25.0 - mean * mean, 0.0));
    float brightnessMean = brightnessSum / 25.0;
    float spatialVariance = max(brightnessSquares / 25.0 - brightnessMean * brightnessMean, 0.0);

    // Where this surface was on screen last frame
    int2 viewPixel = DnViewPixel(pixel);
    float2 uv = (float2(viewPixel) + 0.5) * g_DnViewportSize.zw;
    float3 worldPos = g_DnCameraPos.xyz + normalize(DnViewDirection(float2(viewPixel) + 0.5)) * guide.z;
    float3 N = OctDecode(guide.xy);
    float2 velocity = g_DnVelocity.Load(int3(viewPixel, 0)).xy;

    BilinearTaps taps;
    taps.Base = int2(0, 0);
    taps.Weights = float4(0.0, 0.0, 0.0, 0.0);
    taps.WeightSum = 0.0;

    if (g_DnHistoryValid > 0.5)
    {
        // A sharp reflection is an image of something behind the surface: when the camera moves it slides across
        // the surface, so last frame's value for it is not where the surface point was but where the reflected
        // point was, which is what gets looked up here. (Only for surfaces that stand still: the motion vector must
        // be the camera's alone.)
        if (g_DnKind == SIGNAL_REFLECTION && g_DnVirtualMotion > 0.5 && guide.w < VIRTUAL_MOTION_MAX_ROUGHNESS && current.a > 0.0)
        {
            float4 surfaceNow = mul(float4(worldPos, 1.0), g_DnViewProjNoJitter);
            float4 surfaceThen = mul(float4(worldPos, 1.0), g_DnPrevViewProjNoJitter);
            float3 imagePos = worldPos + normalize(worldPos - g_DnCameraPos.xyz) * current.a;
            float4 imageNow = mul(float4(imagePos, 1.0), g_DnViewProjNoJitter);
            float4 imageThen = mul(float4(imagePos, 1.0), g_DnPrevViewProjNoJitter);

            if (surfaceNow.w > 0.0 && surfaceThen.w > 0.0 && imageNow.w > 0.0 && imageThen.w > 0.0)
            {
                float2 cameraVelocity = ClipToUV(surfaceNow) - ClipToUV(surfaceThen);
                if (length((velocity - cameraVelocity) * g_DnViewportSize.xy) < 0.5)
                {
                    // The pixel found there shows another point of the same reflector, so its distance may differ more
                    float2 imageVelocity = ClipToUV(imageNow) - ClipToUV(imageThen);
                    taps = FindHistory(uv - imageVelocity, N, guide.w, guide.z, guide.z, 0.15);
                }
            }
        }

        // Everything else stays on its surface. A surface that stands still was where the camera's motion puts it;
        // one that moves on its own was at about today's distance.
        if (taps.WeightSum <= 0.01)
        {
            taps = FindHistory(uv - velocity, N, guide.w, length(worldPos - g_DnPrevCameraPos.xyz), guide.z, 0.05);
        }
    }

    float4 history = current;
    float historyLength = 0.0;
    float2 historyMoments = float2(0.0, 0.0);
    if (taps.WeightSum > 0.01)
    {
        history = ZeroNonFinite(LoadBilinearTaps(g_DnPrevSignal, taps));
        float4 moments = LoadBilinearTaps(g_DnPrevMoments, taps);
        historyLength = moments.x;
        historyMoments = moments.yz;

        if (g_DnClampSigma > 0.0)
        {
            float4 clamped = clamp(history, min(mean - g_DnClampSigma * deviation, nearMin), max(mean + g_DnClampSigma * deviation, nearMax));

            // A value that had to be pulled far no longer describes this pixel: new frames then count for more again
            float4 change = abs(clamped - history);
            float pulled = g_DnKind == SIGNAL_VISIBILITY
                ? max(max(change.x, change.y), max(change.z, change.w))
                : Luminance(change.rgb) / (brightnessMean + 0.05);
            historyLength *= 1.0 - saturate(2.0 * pulled);
            history = clamped;
        }
    }

    // The first frame is the ray alone; after that every frame counts the same until the history is long enough,
    // then new frames get a fixed share
    historyLength = min(historyLength + 1.0, MAX_HISTORY_LENGTH);
    float blend = max(1.0 / historyLength, g_DnMinBlend);

    Out.Signal = lerp(history, current, blend);

    float brightness = MomentBrightness(current);
    float2 moments = lerp(historyMoments, float2(brightness, brightness * brightness), blend);
    Out.Moments = float4(historyLength, moments, 0.0);

    // How many rays the result is worth: the frames gathered, up to what a fixed share per frame can hold
    float effectiveSamples = min(historyLength, (2.0 - g_DnMinBlend) / max(g_DnMinBlend, 0.001));

    if (g_DnKind == SIGNAL_VISIBILITY)
    {
        // A ray is 0 or 1, so a pixel that is lit a share p of the time has rays with variance p (1 - p).
        // p is taken from the accumulated value and from this frame's neighbourhood, whichever shows more doubt:
        // the first alone would call a new pixel that happened to draw a 1 certain.
        float4 p = saturate(Out.Signal);
        float4 q = saturate(mean);
        Out.Variance = max(p * (1.0 - p), q * (1.0 - q)) / effectiveSamples;
    }
    else
    {
        // The spread of this pixel's own rays over time; until there are enough of them, its neighbours' rays stand in
        float temporalVariance = max(moments.y - moments.x * moments.x, 0.0);
        float rayVariance = historyLength < SETTLED_HISTORY_LENGTH ? max(temporalVariance, spatialVariance) : temporalVariance;
        float variance = rayVariance / effectiveSamples;
        Out.Variance = float4(variance, variance, variance, variance);
    }
}
