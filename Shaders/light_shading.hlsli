#ifndef LIGHT_SHADING_HLSLI
#define LIGHT_SHADING_HLSLI

// Shading a surface with the scene's lights: sun disks, spheres, spot cones and rectangular panels. Shared by every
// pipeline that lights with the material model of pbr_defs.hlsli (Hybrid, Deferred Hybrid, hit shading, BasicLit).
// Requires light_struct.hlsli and pbr_defs.hlsli to be included first.

// Where a round light's highlight comes from. A glossy surface mirrors the light's body; the part of it the highlight
// shows is the point of the body nearest the reflected view ray (Karis, "Real Shading in Unreal Engine 4").
//   L:        unit vector from the surface to the center of the body
//   tanAngle: tangent of the angle the body's radius spans seen from the surface (radius / distance)
//   R:        the view ray reflected about the normal
float3 RoundLightHighlightDirection(float3 L, float tanAngle, float3 R)
{
    float3 centerToRay = dot(L, R) * R - L;
    float3 closest = L + centerToRay * saturate(tanAngle / max(length(centerToRay), 0.0001));
    return normalize(closest);
}

// What the project's settings make of a light's size on the highlight: the tangent of its angular radius when the
// highlight takes the size into account, else 0, which is a point of light
float LightHighlightWidth(float tanAngle)
{
    return (g_LightFlags & LIGHT_FLAG_HIGHLIGHT_SHAPE) != 0u ? tanAngle : 0.0;
}

// A light with a round body: a directional light (a disk in the sky; tanAngle is the tangent of its angular radius)
// or a point or spot light (a sphere; tanAngle is radius / distance). radiance is what the light delivers to a
// surface facing it; the cosine is applied here.
float3 ShadeRoundLight(float3 N, float3 V, float3 L, float tanAngle, float3 radiance, float3 albedo, float metallic,
    float roughness, float3 F0, float shadowFactor, float3 specularScale)
{
    float width = LightHighlightWidth(tanAngle);
    float3 Ls = L;
    if (width > 0.0)
    {
        Ls = RoundLightHighlightDirection(L, width, reflect(-V, N));
    }
    // The light's own surface: its irradiance spread over the disk it covers in the sky
    float3 lightRadiance = radiance / (PI * max(width * width, 0.000001));
    return ShadeLightSource(N, V, Ls, width, radiance * max(dot(N, L), 0.0), radiance, lightRadiance, albedo, metallic, roughness, F0, shadowFactor, specularScale);
}

// The same with the layers over the material: its clear coat's highlight and its sheen are added, and what is below them
// is dimmed by what they take. Costs nothing for a surface without layers.
float3 ShadeRoundLight(float3 N, float3 V, float3 L, float tanAngle, float3 radiance, float3 albedo, float metallic,
    float roughness, float3 F0, float shadowFactor, float3 specularScale, SurfaceLayers layers)
{
    float3 base = ShadeRoundLight(N, V, L, tanAngle, radiance, albedo, metallic, roughness, F0, shadowFactor, specularScale);
    if (!HasLayers(layers))
    {
        return base;
    }

    float width = LightHighlightWidth(tanAngle);
    float3 lightRadiance = radiance / (PI * max(width * width, 0.000001));

    float sheenScale = 1.0;
    float3 sheen = float3(0.0, 0.0, 0.0);
    if (HasSheen(layers))
    {
        sheen = SheenLight(N, V, L, radiance * max(dot(N, L), 0.0), layers, shadowFactor, sheenScale);
    }

    float coatScale = 1.0;
    float3 coat = float3(0.0, 0.0, 0.0);
    if (HasClearcoat(layers))
    {
        float3 Lc = width > 0.0 ? RoundLightHighlightDirection(L, width, reflect(-V, layers.CoatNormal)) : L;
        coat = ClearcoatLight(V, Lc, width, radiance, lightRadiance, layers, shadowFactor, coatScale);
    }
    return (base * sheenScale + sheen) * coatScale + coat;
}

// What the point, spot or rectangular light j does to a surface point, ready for ShadeLightSource
struct LocalLightSample
{
    float3 L;                   // Unit vector from the surface to the light's center, for the shadow ray
    float Distance;             // To the light's center
    float SourceRadius;         // Radius of the sphere (or of a disk as big as the panel) that stands for the light's body, for the shadow ray
    float3 HighlightDirection;  // Where the highlight comes from
    float HighlightWidth;       // Tangent of the angle the body spans; 0 for a point
    float3 DiffuseIrradiance;   // What the diffuse part is lit by (the cosine included), shadows not applied
    float3 SpecularRadiance;    // What the highlight is lit by, shadows not applied
    float3 LightRadiance;       // Radiance of the light's own surface, which the highlight cannot exceed
};

// Fills in what light j delivers to the surface point p, with shading normal N seen from V (unit vectors).
// False when it adds nothing there: out of range, outside its cone, facing the wrong way (for a panel, the whole panel is
// below the horizon).
bool EvaluateLocalLight(int j, float3 p, float3 N, float3 V, out LocalLightSample s)
{
    s = (LocalLightSample)0;

    if (!LocalLightReaches(j, p, s.L, s.Distance))
        return false;

    // A light that is a point or a sphere adds nothing from behind the surface. A panel can still reach it with its near edge.
    if (g_PointLights[j].RectArea <= 0.0 && dot(N, s.L) <= 0.0)
        return false;

    float3 color = g_PointLights[j].Color.rgb * g_PointLights[j].Color.a;
    float3 R = reflect(-V, N);

    if (g_PointLights[j].RectArea > 0.0)
    {
        float window = LightRangeWindow(s.Distance, g_PointLights[j].Range);
        float irradianceFactor = RectLightIrradianceFactor(j, p, N);
        if (irradianceFactor <= 0.0)
            return false;

        s.DiffuseIrradiance = color * (window * irradianceFactor);
        // The highlight is lit as much as the panel is: its irradiance for a surface facing the center
        s.SpecularRadiance = s.DiffuseIrradiance / max(dot(N, s.L), 0.1);

        // A panel is a disk as big as it is, for the width of the highlight and the softness of the shadow
        s.SourceRadius = sqrt(g_PointLights[j].RectArea / PI);
        float width = LightHighlightWidth(s.SourceRadius / s.Distance);
        s.HighlightDirection = s.L;
        s.LightRadiance = color * window;
        if (width > 0.0)
        {
            s.HighlightDirection = normalize(RectLightClosestPoint(j, p, R) - p);
            s.HighlightWidth = width;
        }
        return true;
    }

    float attenuation = PointLightFalloff(s.Distance, g_PointLights[j].Range, g_PointLights[j].Position.w) * SpotConeFactor(j, s.L);
    s.SourceRadius = g_PointLights[j].Position.w;
    s.DiffuseIrradiance = color * (attenuation * max(dot(N, s.L), 0.0));
    s.SpecularRadiance = color * attenuation;

    float width = LightHighlightWidth(s.SourceRadius / s.Distance);
    s.HighlightDirection = s.L;
    if (width > 0.0)
    {
        s.HighlightDirection = RoundLightHighlightDirection(s.L, width, R);
        s.HighlightWidth = width;
        // A sphere of this radius that delivers SpecularRadiance seen from here shines with this radiance
        s.LightRadiance = s.SpecularRadiance / (PI * max(width * width, 0.000001));
    }
    return true;
}

// Shading with a sample of EvaluateLocalLight
float3 ShadeLocalLight(float3 N, float3 V, LocalLightSample s, float3 albedo, float metallic, float roughness, float3 F0,
    float shadowFactor, float3 specularScale)
{
    return ShadeLightSource(N, V, s.HighlightDirection, s.HighlightWidth, s.DiffuseIrradiance, s.SpecularRadiance, s.LightRadiance,
        albedo, metallic, roughness, F0, shadowFactor, specularScale);
}

// The same with the layers over the material, see ShadeRoundLight. j and p: the light and the surface point the sample was
// made for (EvaluateLocalLight), which the coat needs to find where its own highlight comes from.
float3 ShadeLocalLight(float3 N, float3 V, LocalLightSample s, float3 albedo, float metallic, float roughness, float3 F0,
    float shadowFactor, float3 specularScale, SurfaceLayers layers, int j, float3 p)
{
    float3 base = ShadeLocalLight(N, V, s, albedo, metallic, roughness, F0, shadowFactor, specularScale);
    if (!HasLayers(layers))
    {
        return base;
    }

    float sheenScale = 1.0;
    float3 sheen = float3(0.0, 0.0, 0.0);
    if (HasSheen(layers))
    {
        sheen = SheenLight(N, V, s.L, s.DiffuseIrradiance, layers, shadowFactor, sheenScale);
    }

    float coatScale = 1.0;
    float3 coat = float3(0.0, 0.0, 0.0);
    if (HasClearcoat(layers))
    {
        // The highlight is where the coat's own mirror direction meets the light's body
        float3 Lc = s.L;
        if (s.HighlightWidth > 0.0)
        {
            float3 Rc = reflect(-V, layers.CoatNormal);
            Lc = g_PointLights[j].RectArea > 0.0 ? normalize(RectLightClosestPoint(j, p, Rc) - p) : RoundLightHighlightDirection(s.L, s.HighlightWidth, Rc);
        }
        coat = ClearcoatLight(V, Lc, s.HighlightWidth, s.SpecularRadiance, s.LightRadiance, layers, shadowFactor, coatScale);
    }
    return (base * sheenScale + sheen) * coatScale + coat;
}

#endif // LIGHT_SHADING_HLSLI
