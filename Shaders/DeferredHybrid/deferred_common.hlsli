#ifndef DEFERRED_HYBRID_COMMON_HLSLI
#define DEFERRED_HYBRID_COMMON_HLSLI

// Shared by the Deferred Hybrid passes that start from the G-buffer (DeferredHybridPipeline::FinishFrame):
//
//   guide_ps             each pixel's normal, distance and roughness, kept for a frame so the denoisers can tell
//                        which pixels show the same surface
//   shadow_trace_ps      one shadow ray per light                                  -> "shadow signal"
//   ao_trace_ps          one ambient occlusion ray (GI method Sky + AO)            -> "indirect signal"
//   gi_trace_ps          the per-pixel GI rays (GI method Per-pixel)               -> "indirect signal"
//   reflection_trace_ps  one reflection ray                                        -> "reflection signal"
//   lighting_ps          composes the image from the G-buffer and the signals; traces nothing
//
// A signal is what one kind of ray found, per pixel. With one ray per pixel it is mostly noise, so between its
// trace pass and the lighting pass each signal goes through SignalDenoiser (denoise_*.hlsl), unless the project's
// Denoising settings switch that off, in which case the lighting pass shows the rays as traced.
//
// Requires light_struct.hlsli, pbr_defs.hlsli, fog.hlsli and hybrid_shading.hlsli to be included first.

#include "gbuffer_common.hlsli"
#include "../Common/math_common.hlsli"
#include "../Common/screen_common.hlsli"

Texture2D g_GBufferAlbedoMetallic;
Texture2D g_GBufferNormals;
Texture2D g_GBufferEmissiveAO;
Texture2D g_GBufferRoughness;
Texture2D g_GBufferDepth;

// Mirrors DeferredHybridConstants in DeferredHybridPipeline.cpp
cbuffer DeferredHybridConstants
{
    float4x4 g_InvViewProj; // This frame's clip space -> world (the matrix the G-buffer was drawn with)
    float4 g_ViewportSize;  // xy: size in pixels, zw: 1 / size
    float4 g_ViewParams;    // x: angle one pixel covers, in radians
    float4 g_ViewOrigin;    // xyz: where the camera is. The guide's distances are measured from here.
    float4 g_GIParams;      // x: 1 when the indirect signal is per-pixel GI, 0 when it is ambient occlusion
                            // y: view pixels per pixel of the indirect signal (1 or 2), z: GI intensity

    uint g_SignalFrame;     // Counts up every frame: moves the rays of the signals a denoiser averages
    uint g_DenoiseFlags;    // DENOISE_* bits: the signals that go through a denoiser this frame
    uint g_ShadowLayer;     // shadow_trace_ps: which four light slots this draw covers (slots 4 * layer onward)
    uint g_GlassFlags;      // GLASS_* bits below
};

static const uint DENOISE_SHADOWS = 1u;
static const uint DENOISE_INDIRECT = 2u;
static const uint DENOISE_REFLECTIONS = 4u;

// Bits of g_GlassFlags
static const uint GLASS_TINT_SHADOWS = 1u;  // The shadow pass wrote the color glass tints the direct light with (g_GlassTint)

// The guide: xy = octahedral geometric normal, z = distance from the camera, w = roughness (0: sky, nothing drawn).
// Same size as the view.
Texture2D g_Guide;

// rgb + a, at 1 / g_GIParams.y of the view's size. Per-pixel GI: indirect light arriving, and how open the
// surroundings are. Sky + AO: average direction of the occlusion rays that got away, and the share that did.
Texture2D g_IndirectSignal;

// The surface a pixel shows, rebuilt from the G-buffer. Returns false for the sky (nothing was drawn there).
// The indirect light fields are left for ApplyIndirectSignal.
bool LoadGBufferSurface(int2 pixel, out HybridSurface surface, out float depth)
{
    surface = (HybridSurface) 0;

    // The G-buffer can be larger than the viewport (it only grows), so fetch by pixel instead of by UV
    int3 texel = int3(pixel, 0);
    depth = g_GBufferDepth.Load(texel).r;
    if (depth >= 1.0)
    {
        return false;
    }

    // World position from depth, and how far off it can be (see DepthPositionError): the ray origins are pushed
    // out by that much on top of the shadow bias
    float2 uv = (float2(pixel) + 0.5) * g_ViewportSize.zw;
    float4 clipPos = UVToClip(uv, depth);
    float3 worldPos = ClipToWorld(clipPos, g_InvViewProj);

    float4 albedoMetallic = g_GBufferAlbedoMetallic.Load(texel);
    float4 normals = g_GBufferNormals.Load(texel);
    float4 emissiveAO = g_GBufferEmissiveAO.Load(texel);

    surface.WorldPos = worldPos;
    surface.N = OctDecode(normals.xy);
    surface.GeoN = OctDecode(normals.zw);
    surface.PositionError = DepthPositionError(clipPos, worldPos, g_InvViewProj);
    surface.Albedo = albedoMetallic.rgb;
    surface.Metallic = albedoMetallic.a;
    surface.Roughness = g_GBufferRoughness.Load(texel).r;
    surface.AO = emissiveAO.a;
    surface.Emissive = emissiveAO.rgb;
    surface.PixelSpread = g_ViewParams.x;
    surface.UseIndirectLight = false;
    surface.IndirectLight = float4(0.0, 0.0, 0.0, 1.0);
    return true;
}

// The pixel's random numbers for this frame's rays. A signal that goes through a denoiser gets new ones every
// frame (the denoiser averages them); one that is shown as traced only does under TAA, see RayNoise.
float2 SignalNoise(float2 pixelPos, uint denoiseBit)
{
    if ((g_DenoiseFlags & denoiseBit) != 0u)
    {
        s_RaysDenoised = true;
        return HashNoise(int2(pixelPos), g_SignalFrame);
    }
    return RayNoise(pixelPos);
}

// The indirect signal for a view pixel. At half resolution each of its pixels covers 2x2 view pixels, so the four
// around this pixel are blended, leaving out the ones that lie on another surface (the guide tells): simply
// stretching the signal would smear light across object edges.
float4 SampleIndirectSignal(int2 pixel, float3 geoN, float viewDistance)
{
    int scale = max(int(g_GIParams.y), 1);
    if (scale == 1)
    {
        return g_IndirectSignal.Load(int3(pixel, 0));
    }

    int2 base = pixel / scale;
    float2 f = float2(pixel - base * scale) / float(scale);
    int2 maxViewPixel = int2(g_ViewportSize.xy) - 1;
    int2 maxTexel = (int2(g_ViewportSize.xy) + scale - 1) / scale - 1;

    float4 sum = float4(0.0, 0.0, 0.0, 0.0);
    float weightSum = 0.0;
    for (int j = 0; j < 2; ++j)
    {
        for (int i = 0; i < 2; ++i)
        {
            float w = (i == 0 ? 1.0 - f.x : f.x) * (j == 0 ? 1.0 - f.y : f.y);
            if (w <= 0.0)
                continue;

            // A signal pixel stands for the view pixel at scale times its position: that is where its guide is
            int2 texel = min(base + int2(i, j), maxTexel);
            float4 guide = g_Guide.Load(int3(min(texel * scale, maxViewPixel), 0));
            if (guide.w <= 0.0)
                continue;
            w *= pow(saturate(dot(geoN, OctDecode(guide.xy))), 8.0);
            w *= exp(-abs(guide.z - viewDistance) / (0.05 * viewDistance + 0.01));

            sum += g_IndirectSignal.Load(int3(texel, 0)) * w;
            weightSum += w;
        }
    }

    // None of the four matched (a thin object between signal pixels): the nearest one is the best guess left
    if (weightSum < 0.0001)
    {
        return g_IndirectSignal.Load(int3(min(base, maxTexel), 0));
    }
    return sum / weightSum;
}

// Reads the indirect signal into the surface (per-pixel GI) or into the ambient occlusion results (Sky + AO),
// ready for GetHybridAmbient / ComposeHybridSurface
void ApplyIndirectSignal(inout HybridSurface surface, int2 pixel, out float ambientVisibility, out float3 openDirection)
{
    float4 indirect = SampleIndirectSignal(pixel, surface.GeoN, length(surface.WorldPos - g_ViewOrigin.xyz));

    ambientVisibility = 1.0;
    openDirection = surface.N;
    surface.UseIndirectLight = g_GIParams.x > 0.5;
    if (surface.UseIndirectLight)
    {
        surface.IndirectLight = float4(SanitizeLight(indirect.rgb) * g_GIParams.z, saturate(indirect.a));
    }
    else
    {
        ambientVisibility = saturate(indirect.a);
        openDirection = indirect.rgb;
    }
}

#endif // DEFERRED_HYBRID_COMMON_HLSLI
