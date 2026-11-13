// gi_accumulate_ps.hlsl
// Per-pixel diffuse GI, pass 2 of 3 (DiffuseGIPass): averages rays over time. Paired with Deferred/fullscreen_vs.hlsl.
//
// Each pixel finds where its surface was on screen last frame and blends this frame's ray into the result stored
// there. The more frames a pixel has gathered, the less each new ray changes it, down to g_GIMinBlend, so the image
// settles while still following changes in the lighting.
//
// Last frame's result is only used where it shows the same surface: its guide must have a matching distance and
// normal. Where it doesn't (something moved in front, the camera turned to a new area) the pixel starts over.
// There are no motion vectors yet, so a moving object is looked up where it is now, not where it was.
#include "gbuffer_common.hlsli"
#include "gi_common.hlsli"

Texture2D g_GITrace;                // This frame's rays
Texture2D g_GIGuide;                // This frame's guide
Texture2D g_GIPrevHistoryLength;    // (Last frame's result and guide are declared in gi_common.hlsli)

struct GIAccumulateOutput
{
    float4 Light : SV_TARGET0;          // Same layout as the trace output, averaged over time
    float HistoryLength : SV_TARGET1;   // Frames this pixel has been gathering light
};

static const float GI_MAX_HISTORY_LENGTH = 255.0;


void main_ps(in FullScreenPSInput In, out GIAccumulateOutput Out)
{
    int2 giPixel = int2(In.Pos.xy);

    Out.Light = float4(0.0, 0.0, 0.0, 1.0);
    Out.HistoryLength = 0.0;

    float4 guide = g_GIGuide.Load(int3(giPixel, 0));
    if (guide.w < 0.5)
    {
        return; // Sky
    }

    float4 current = g_GITrace.Load(int3(giPixel, 0));
    float3 N = OctDecode(guide.xy);
    float3 worldPos = g_GICameraPos.xyz + GIViewRay(GIViewPixel(giPixel)) * guide.z;

    // Last frame's result where this surface was on screen then, if it was
    float4 history = float4(0.0, 0.0, 0.0, 0.0);
    float historyLength = 0.0;

    GILastFrame last = GIFindLastFrame(worldPos, N);
    if (last.WeightSum > 0.01)
    {
        history = GILoadLastFrame(last);
        history.rgb = GISanitize(history.rgb);

        for (int k = 0; k < 4; ++k)
        {
            if (last.Weights[k] > 0.0)
            {
                historyLength += g_GIPrevHistoryLength.Load(int3(last.Base + int2(k & 1, k >> 1), 0)).r * last.Weights[k];
            }
        }
        historyLength /= last.WeightSum;
    }


    // The first frame is the ray alone; after that every frame counts the same until the history is long enough,
    // then new frames get a fixed share
    historyLength = min(historyLength + 1.0, GI_MAX_HISTORY_LENGTH);
    float blend = max(1.0 / historyLength, g_GIMinBlend);

    Out.Light = lerp(history, current, blend);
    Out.HistoryLength = historyLength;
}
