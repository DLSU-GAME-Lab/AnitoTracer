#ifndef RT_SHADOWS_HLSLI
#define RT_SHADOWS_HLSLI

// Ray-traced visibility against the scene TLAS, shared by every pipeline that traces shadow rays
// (hybrid_shading.hlsli, CrossHatch/crosshatch_ps.hlsl).
// Requires light_struct.hlsli (g_ShadowBias, g_NoiseFrame), pbr_defs.hlsli (PI) and fog.hlsli (InterleavedGradientNoise) to be included first.

// Master, we bind our Scene Acceleration Structure for Ray Queries here!
RaytracingAccelerationStructure g_TLAS;

// ------------------------------------------------------------------
// Ray budget / quality knobs
// ------------------------------------------------------------------
static const int SHADOW_SAMPLES = 4;            // Rays per light, spread over the light's area
static const float SUN_ANGULAR_RADIUS = 0.01;   // Radians. Real sun is ~0.0047, slightly larger reads better without denoising
static const float POINT_LIGHT_RADIUS = 0.1;    // World units, gives point lights a soft penumbra

// ------------------------------------------------------------------
// Sampling helpers
// ------------------------------------------------------------------

// Rotation (x) and radial offset (y) for one pixel's sample pattern, both in [0, 1).
// While TAA accumulates frames (g_NoiseFrame > 0) the pattern changes every frame, so the
// accumulated result covers many more samples than are traced per frame. Without TAA it stays
// still (radial offset 0.5, the evenly spread pattern), since a changing pattern would only flicker.
float2 RayNoise(float2 pixelPos)
{
    if (g_NoiseFrame == 0)
    {
        return float2(InterleavedGradientNoise(pixelPos), 0.5);
    }
    float2 frameOffset = 5.588238 * float(g_NoiseFrame % 64);
    return float2(InterleavedGradientNoise(pixelPos + frameOffset),
                  InterleavedGradientNoise(pixelPos + float2(47.0, 17.0) + frameOffset));
}

// Same noise, decorrelated for the k-th light or effect so their patterns don't line up (golden ratio shift)
float2 ShiftNoise(float2 noise, int k)
{
    return float2(frac(noise.x + 0.618034 * k), frac(noise.y + 0.754878 * k));
}

// Point i of n on a unit disk, evenly spread, rotated by phi (radians).
// radialOffset in [0, 1) places the sample within its ring; 0.5 is the ring's middle.
float2 VogelDisk(int i, int n, float phi, float radialOffset)
{
    float r = sqrt((i + radialOffset) / n);
    float theta = i * 2.39996323 + phi; // Golden angle
    return r * float2(cos(theta), sin(theta));
}

void BuildBasis(float3 n, out float3 t, out float3 b)
{
    float3 up = abs(n.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

// ------------------------------------------------------------------
// Helper: Traces a visibility ray, returns 1 when nothing is hit
// positionError: how far worldPos may be off the real surface (deferred depth reconstruction), added to the bias
// ------------------------------------------------------------------
float TraceShadowRay(float3 worldPos, float3 normal, float positionError, float3 lightDir, float maxDist)
{
    // Master! We offset the ray origin along the unperturbed geometric normal to prevent self-shadowing~♡
    RayDesc ray;
    ray.Origin = worldPos + (normal * (g_ShadowBias + positionError));
    ray.Direction = lightDir;
    ray.TMin = 0.001;
    ray.TMax = maxDist;

    RayQuery < RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER > q;

    // Force opaque so the RayQuery doesn't wait for any-hit shader confirmation!
    q.TraceRayInline(g_TLAS, RAY_FLAG_FORCE_OPAQUE, 0xFF, ray);

    // Traversal state machine loop~
    while (q.Proceed())
    {
    }

    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        return 0.0;
    }

    return 1.0;
}

// Soft shadow from a directional light treated as a disk of SUN_ANGULAR_RADIUS. noise comes from RayNoise
float TraceDirectionalShadow(float3 worldPos, float3 geoN, float positionError, float3 L, float2 noise)
{
    float3 t, b;
    BuildBasis(L, t, b);
    float spread = tan(SUN_ANGULAR_RADIUS);

    float visibility = 0.0;
    for (int s = 0; s < SHADOW_SAMPLES; ++s)
    {
        float2 d = VogelDisk(s, SHADOW_SAMPLES, noise.x * 2.0 * PI, noise.y) * spread;
        visibility += TraceShadowRay(worldPos, geoN, positionError, normalize(L + t * d.x + b * d.y), 1000.0);
    }
    return visibility / SHADOW_SAMPLES;
}

// Soft shadow from a point light treated as a disk of POINT_LIGHT_RADIUS facing the surface. noise comes from RayNoise
float TracePointShadow(float3 worldPos, float3 geoN, float positionError, float3 lightPos, float3 L, float2 noise)
{
    float3 t, b;
    BuildBasis(L, t, b);

    float visibility = 0.0;
    for (int s = 0; s < SHADOW_SAMPLES; ++s)
    {
        float2 d = VogelDisk(s, SHADOW_SAMPLES, noise.x * 2.0 * PI, noise.y) * POINT_LIGHT_RADIUS;
        float3 toSample = lightPos + t * d.x + b * d.y - worldPos;
        float dist = length(toSample);
        visibility += TraceShadowRay(worldPos, geoN, positionError, toSample / dist, dist - 0.01);
    }
    return visibility / SHADOW_SAMPLES;
}

#endif // RT_SHADOWS_HLSLI
