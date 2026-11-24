#ifndef SAMPLING_COMMON_HLSLI
#define SAMPLING_COMMON_HLSLI

// Pseudo-random numbers and sample patterns, for rays and for dithering. Needs nothing else, so it can be included first.

// Per-pixel value in [0, 1) with no visible pattern (interleaved gradient noise). Used to offset ray-march samples,
// to rotate a pixel's sample pattern and to dither: it trades visible banding for grain too fine to see.
float InterleavedGradientNoise(float2 pixelPos)
{
    return frac(52.9829189 * frac(dot(pixelPos, float2(0.06711056, 0.00583715))));
}

// Scrambles an integer into another with no visible pattern (PCG hash)
uint HashUint(uint v)
{
    uint state = v * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// Point i of n on a unit disk, evenly spread, rotated by phi (radians).
// radialOffset in [0, 1) places the sample within its ring; 0.5 is the ring's middle.
float2 VogelDisk(int i, int n, float phi, float radialOffset)
{
    float r = sqrt((i + radialOffset) / n);
    float theta = i * 2.39996323 + phi; // Golden angle
    return r * float2(cos(theta), sin(theta));
}

// A point d on a unit disk, lifted onto the hemisphere around N (t and b are two directions at right angles to N, see
// BuildBasis). Points spread evenly over the disk give a cosine-weighted distribution (Malley's method), which is how a
// matte surface collects light. Not normalized: it shortens a little toward the rim of the hemisphere's base.
float3 CosineHemisphereDirection(float3 N, float3 t, float3 b, float2 d)
{
    return t * d.x + b * d.y + N * sqrt(saturate(1.0 - dot(d, d)));
}

// Normal maps can tilt a direction below the real surface; mirror it back above it
float3 KeepAboveSurface(float3 dir, float3 geoN)
{
    float below = dot(dir, geoN);
    if (below < 0.0)
    {
        dir -= 2.0 * below * geoN;
    }
    return dir;
}

#endif // SAMPLING_COMMON_HLSLI
