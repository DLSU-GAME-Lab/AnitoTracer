#include "Common/math_common.hlsli"

struct DirectionalLight
{
    float4 Direction; // xyz: Light direction, w: angular radius of the light's disk, in radians
    float4 Color;     // rgb: Color, a: Intensity
};

// Point, spot and rectangular lights share this struct (mirrors PointLightData in PipelineDefs.hpp). Every row is
// 16 bytes, so nothing needs padding.
struct PointLight
{
    float4 Position;        // xyz: Position, w: radius of the light's body
    float4 Color;           // rgb: Color, a: Intensity (a rectangular light: the radiance of its panel)
    float4 SpotDirection;   // xyz: the way a spot light points, or the side of a rectangular light that shines
    float Range;            // Max attenuation distance
    float SpotScale;        // A spot light's cone factor is saturate(dot(-L, SpotDirection) * SpotScale + SpotOffset)^2.
    float SpotOffset;       // A point light has scale 0 and offset 1, which makes it 1 in every direction.
    float RectArea;         // m2; above 0 for a rectangular light
    float4 AxisX;           // Rectangular light: half of its width, as a world vector from its center to an edge
    float4 AxisY;           // And half of its height
};

cbuffer LightConstants
{
    DirectionalLight g_DirLights[4];
    PointLight g_PointLights[8];
    int g_NumDirLights;
    int g_NumPointLights;
    uint g_LightFlags;      // LIGHT_FLAG_* bits, below
    float g_LightPadding;
    float4 g_CameraPos; // World space camera position
};

cbuffer ShadowSettings
{
    float g_ShadowBias;
    float g_AmbientMultiplier;
    float g_SkyLightIntensity;   // Hybrid only: strength of the sky's light (sky intensity * sky light intensity)
    float g_ReflectionIntensity; // Hybrid only: strength of sky reflections
    float4 g_AmbientColor;
    uint g_NoiseFrame;           // Changes every frame while TAA is on, so ray sample patterns vary over time; 0 keeps them still
    float2 g_SkyRotation;        // Hybrid only: (sin, cos) of the sky's rotation around the up axis, see RotateSkyDirection
    float g_ReflectionMaxRoughness; // Hybrid only: surfaces rougher than this reflect the sky without tracing a ray

    float g_ReflectionMaxDistance;  // Hybrid only: how far a reflection ray looks for geometry
    uint g_ReflectionFlags;         // Hybrid only: REFLECTION_* bits, see hybrid_shading.hlsli
    uint g_TransparencyFlags;       // TRANSPARENCY_* bits, see rt_shadows.hlsli
    float g_ShadowPadding;
};

// ------------------------------------------------------------------
// Where the lights are and how far they reach: falloff, spot cones and rectangular lights.
// Shading with a material is in light_shading.hlsli.
// ------------------------------------------------------------------

// Bits of g_LightFlags, from the project's physical light settings (mirror LightFlags in PipelineDefs.hpp)
static const uint LIGHT_FLAG_INVERSE_SQUARE = 1u;   // Point and spot lights fall off as 1 / distance^2, cut smoothly at their range
static const uint LIGHT_FLAG_HIGHLIGHT_SHAPE = 2u; // A light's size widens the highlight it makes on glossy surfaces

// Smooth cut that takes a falloff to 0 at the light's range (Karis, "Real Shading in Unreal Engine 4")
float LightRangeWindow(float dist, float range)
{
    float r = dist / max(range, 0.0001);
    float window = saturate(1.0 - r * r * r * r);
    return window * window;
}

// What is left of a point or spot light's intensity at a distance: 1 / d^2 inside its range (the window cuts it smoothly
// at the end) or, in the old falloff, (1 - d / range)^2. A light has a body, which keeps the inverse square from
// growing without limit as a surface touches it.
float PointLightFalloff(float dist, float range, float bodyRadius)
{
    if ((g_LightFlags & LIGHT_FLAG_INVERSE_SQUARE) != 0u)
    {
        return LightRangeWindow(dist, range) / max(dist * dist, max(bodyRadius * bodyRadius, 0.01));
    }

    float a = max(1.0 - dist / range, 0.0);
    return a * a;
}

// A spot light's cone: 1 inside its inner angle, 0 outside its outer angle, a smooth fade between. L points from the
// surface to the light. Point lights have no cone, so this is 1 for them.
float SpotConeFactor(int j, float3 L)
{
    float t = saturate(dot(-L, g_PointLights[j].SpotDirection.xyz) * g_PointLights[j].SpotScale + g_PointLights[j].SpotOffset);
    return t * t;
}

// Whether point, spot or rectangular light j adds anything at p: within its range, inside its cone, on the lit side of
// its panel. Gives the direction to the light's center (unit length) and the distance to it.
bool LocalLightReaches(int j, float3 p, out float3 L, out float dist)
{
    float3 lightVec = g_PointLights[j].Position.xyz - p;
    dist = length(lightVec);
    L = lightVec / max(dist, 0.0001);
    if (dist >= g_PointLights[j].Range || dist <= 0.0001)
        return false;

    if (g_PointLights[j].RectArea > 0.0)
        return dot(-L, g_PointLights[j].SpotDirection.xyz) > 0.0; // A panel shines on one side only

    return SpotConeFactor(j, L) > 0.0;
}

// The light of point, spot or rectangular light j at p, for shading that treats every light as a point: how many
// times its Color * Intensity it delivers to a surface facing it (the cosine is left to the caller). A rectangular
// light counts as a point that is as bright as the panel is large and shines like a flat emitter.
// L and dist are filled in as by LocalLightReaches; 0 when the light adds nothing.
float LocalLightAttenuation(int j, float3 p, out float3 L, out float dist)
{
    if (!LocalLightReaches(j, p, L, dist))
        return 0.0;

    if (g_PointLights[j].RectArea > 0.0)
    {
        float facing = saturate(dot(-L, g_PointLights[j].SpotDirection.xyz));
        return LightRangeWindow(dist, g_PointLights[j].Range) * g_PointLights[j].RectArea * facing / max(dist * dist, 0.01);
    }

    return PointLightFalloff(dist, g_PointLights[j].Range, g_PointLights[j].Position.w) * SpotConeFactor(j, L);
}

// One edge of Lambert's polygon formula: the angle the edge spans seen from the origin, around the axis the origin and
// the edge define, weighted by how much that axis lines up with the surface normal
float RectEdgeTerm(float3 a, float3 b, float3 N)
{
    float3 na = SafeNormalize(a);
    float3 nb = SafeNormalize(b);
    float3 axis = cross(na, nb);
    float axisLength = length(axis);
    if (axisLength < 1e-6)
        return 0.0;

    return acos(clamp(dot(na, nb), -1.0, 1.0)) * dot(axis / axisLength, N);
}

// What a rectangular light j delivers to a point p facing N, per unit of its radiance: the cosine-weighted solid angle
// of the panel above the point's horizon, so E = radiance * this. Lambert's polygon formula over the panel's corners,
// after cutting them to the half space above the horizon, as in Lagarde & de Rousiers, "Moving Frostbite to PBR".
// Exact for a diffuse surface.
float RectLightIrradianceFactor(int j, float3 p, float3 N)
{
    float3 c = g_PointLights[j].Position.xyz - p;
    float3 ax = g_PointLights[j].AxisX.xyz;
    float3 ay = g_PointLights[j].AxisY.xyz;
    if (dot(-c, g_PointLights[j].SpotDirection.xyz) <= 0.0)
        return 0.0;

    float3 corners[4];
    corners[0] = c - ax - ay;
    corners[1] = c + ax - ay;
    corners[2] = c + ax + ay;
    corners[3] = c - ax + ay;

    // Cut the panel to what is above the horizon: Sutherland-Hodgman against one plane, so at most 5 corners remain
    float3 poly[5];
    int n = 0;
    for (int k = 0; k < 4; ++k)
    {
        float3 cur = corners[k];
        float3 nxt = corners[(k + 1) & 3];
        float dc = dot(cur, N);
        float dn = dot(nxt, N);
        if (dc >= 0.0)
        {
            poly[n] = cur;
            n++;
        }
        if ((dc >= 0.0) != (dn >= 0.0))
        {
            poly[n] = lerp(cur, nxt, dc / (dc - dn));
            n++;
        }
    }
    if (n < 3)
        return 0.0;

    float sum = 0.0;
    for (int e = 0; e < n; ++e)
    {
        sum += RectEdgeTerm(poly[e], poly[(e + 1 == n) ? 0 : e + 1], N);
    }
    return 0.5 * abs(sum);
}

// The point of rectangular light j nearest to the line a mirror at p would send light along (R, unit length): the part
// of the panel the highlight comes from (Karis's "representative point", for a rectangle)
float3 RectLightClosestPoint(int j, float3 p, float3 R)
{
    float3 center = g_PointLights[j].Position.xyz;
    float3 n = g_PointLights[j].SpotDirection.xyz;
    float3 ax = g_PointLights[j].AxisX.xyz;
    float3 ay = g_PointLights[j].AxisY.xyz;
    float halfWidth = max(length(ax), 0.0001);
    float halfHeight = max(length(ay), 0.0001);

    // Where the reflected ray meets the panel's plane; a ray that points away from it uses the point straight below p
    float denom = dot(R, n);
    float3 onPlane = p - n * dot(p - center, n);
    if (denom < -0.0001)
    {
        onPlane = p + R * (dot(center - p, n) / denom);
    }

    float3 d = onPlane - center;
    float x = clamp(dot(d, ax / halfWidth), -halfWidth, halfWidth);
    float y = clamp(dot(d, ay / halfHeight), -halfHeight, halfHeight);
    return center + ax / halfWidth * x + ay / halfHeight * y;
}
