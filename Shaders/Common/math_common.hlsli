#ifndef MATH_COMMON_HLSLI
#define MATH_COMMON_HLSLI

// Constants and small math helpers that any shader may use. Needs nothing else, so it can be included first.

static const float PI = 3.14159265359;

// Rec. 709 luminance weights: how much each colour channel adds to brightness
static const float3 LUMINANCE_WEIGHTS = float3(0.2126, 0.7152, 0.0722);

float Luminance(float3 color)
{
    return dot(color, LUMINANCE_WEIGHTS);
}

// True where a value has a NaN or an infinity anywhere in it
bool IsNonFinite(float3 value)
{
    return any(isnan(value)) || any(isinf(value));
}

bool IsNonFinite(float4 value)
{
    return any(isnan(value)) || any(isinf(value));
}

// A value that broke somewhere (a NaN, an infinity, a negative) would spread through every frame after it: black instead
float3 SanitizeLight(float3 light)
{
    return IsNonFinite(light) ? float3(0.0, 0.0, 0.0) : max(light, 0.0);
}

// The same for a signal that may hold negative values: only a NaN or an infinity becomes black
float4 ZeroNonFinite(float4 value)
{
    return IsNonFinite(value) ? float4(0.0, 0.0, 0.0, 0.0) : value;
}

// Unit vector; the zero vector stays zero instead of becoming NaN
float3 SafeNormalize(float3 v)
{
    return v * rsqrt(max(dot(v, v), 1e-12));
}

// Two directions at right angles to n and to each other
void BuildBasis(float3 n, out float3 t, out float3 b)
{
    float3 up = abs(n.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    t = normalize(cross(up, n));
    b = cross(n, t);
}

#endif // MATH_COMMON_HLSLI
