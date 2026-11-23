#ifndef FOG_HLSLI
#define FOG_HLSLI

// Linear + volumetric fog shared by every lit pass and the skybox.
// Requires light_struct.hlsli (LightConstants) to be included first.

cbuffer FogConstants
{
    float4 g_LinearFogColor;     // rgb: color, a: max opacity
    float4 g_VolumetricFogColor; // rgb: scattering tint, a: density
    float g_LinearFogStart;
    float g_LinearFogEnd;
    float g_FogHeightFalloff;
    float g_FogBaseHeight;
    float g_FogAnisotropy;
    float g_FogMaxDistance;
    float g_FogNoiseStrength;
    float g_FogNoiseScale;
    float4 g_FogWind;            // xyz: wind velocity, w: time in seconds
    int g_LinearFogEnabled;
    int g_VolumetricFogEnabled;
    int g_FogApplyToSkybox;
    int g_FogSampleCount;
    float g_FogLightIntensity;
    float g_FogAmbientIntensity;
    uint g_FogNoiseFrame;        // Changes every frame while TAA is on, so the march jitter varies over time; 0 keeps it still
    float g_FogPadding;
};

static const int FOG_MAX_SAMPLES = 128;

// ------------------------------------------------------------------
// Noise helpers
// ------------------------------------------------------------------

// Per-pixel dither used to offset ray-march samples and hide banding
float InterleavedGradientNoise(float2 pixelPos)
{
    return frac(52.9829189 * frac(dot(pixelPos, float2(0.06711056, 0.00583715))));
}

float FogHash(float3 p)
{
    p = frac(p * 0.3183099 + 0.1);
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float FogValueNoise(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    return lerp(
        lerp(lerp(FogHash(i + float3(0, 0, 0)), FogHash(i + float3(1, 0, 0)), f.x),
             lerp(FogHash(i + float3(0, 1, 0)), FogHash(i + float3(1, 1, 0)), f.x), f.y),
        lerp(lerp(FogHash(i + float3(0, 0, 1)), FogHash(i + float3(1, 0, 1)), f.x),
             lerp(FogHash(i + float3(0, 1, 1)), FogHash(i + float3(1, 1, 1)), f.x), f.y),
        f.z);
}

// Two-octave value noise in [0, 1]
float FogNoise(float3 p)
{
    return FogValueNoise(p) * 0.65 + FogValueNoise(p * 2.03 + 17.0) * 0.35;
}

// ------------------------------------------------------------------
// Linear fog
// ------------------------------------------------------------------

float3 ApplyLinearFog(float3 color, float distance)
{
    float range = max(g_LinearFogEnd - g_LinearFogStart, 0.0001);
    float fogAmount = saturate((distance - g_LinearFogStart) / range) * g_LinearFogColor.a;
    return lerp(color, g_LinearFogColor.rgb, fogAmount);
}

// ------------------------------------------------------------------
// Volumetric fog
// ------------------------------------------------------------------

// Exponential height fog, broken up by drifting noise
float FogDensityAt(float3 p)
{
    float height = max(p.y - g_FogBaseHeight, 0.0);
    float density = g_VolumetricFogColor.a * exp(-g_FogHeightFalloff * height);

    if (g_FogNoiseStrength > 0.0)
    {
        float n = FogNoise((p - g_FogWind.xyz * g_FogWind.w) * g_FogNoiseScale);
        density *= max(lerp(1.0, n * 2.0, g_FogNoiseStrength), 0.0);
    }
    return density;
}

// Henyey-Greenstein phase, scaled so isotropic scattering (g = 0) returns 1
float FogPhase(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = max(1.0 + g2 - 2.0 * g * cosTheta, 0.0001);
    return (1.0 - g2) / (denom * sqrt(denom));
}

// Light arriving at a point inside the fog, before tinting
float3 FogInScatteredLight(float3 p, float3 rayDir, bool includePointLights)
{
    float3 light = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < g_NumDirLights; ++i)
    {
        float3 L = normalize(-g_DirLights[i].Direction.xyz);
        float3 radiance = g_DirLights[i].Color.rgb * g_DirLights[i].Color.a;
        light += radiance * FogPhase(dot(rayDir, L), g_FogAnisotropy);
    }

    if (includePointLights)
    {
        for (int j = 0; j < g_NumPointLights; ++j)
        {
            float3 lightVec = g_PointLights[j].Position.xyz - p;
            float dist = length(lightVec);
            if (dist < g_PointLights[j].Range)
            {
                // Same falloff as the surface shaders so fog halos match lit areas
                float attenuation = max(1.0 - (dist / g_PointLights[j].Range), 0.0);
                attenuation *= attenuation;
                float3 radiance = g_PointLights[j].Color.rgb * g_PointLights[j].Color.a * attenuation;
                light += radiance * FogPhase(dot(rayDir, lightVec / max(dist, 0.0001)), g_FogAnisotropy);
            }
        }
    }

    return light * g_FogLightIntensity;
}

float3 ApplyVolumetricFog(float3 color, float3 rayOrigin, float3 rayDir, float rayLength, float2 pixelPos, bool includePointLights)
{
    float marchLength = min(rayLength, g_FogMaxDistance);
    if (marchLength <= 0.0 || g_VolumetricFogColor.a <= 0.0)
    {
        return color;
    }

    int sampleCount = clamp(g_FogSampleCount, 1, FOG_MAX_SAMPLES);
    float stepSize = marchLength / sampleCount;
    // Under TAA the jitter moves every frame, so the accumulated image averages many more march positions
    float jitter = InterleavedGradientNoise(pixelPos + 5.588238 * float(g_FogNoiseFrame % 64));

    float3 tint = g_VolumetricFogColor.rgb;
    float3 ambient = g_FogAmbientIntensity.xxx;

    float transmittance = 1.0;
    float3 scattered = float3(0.0, 0.0, 0.0);

    for (int i = 0; i < sampleCount; ++i)
    {
        float3 p = rayOrigin + rayDir * ((i + jitter) * stepSize);
        float density = FogDensityAt(p);
        if (density <= 0.00001)
        {
            continue;
        }

        float3 lightIn = tint * (ambient + FogInScatteredLight(p, rayDir, includePointLights));

        // Energy-conserving step integration (Hillaire, "Physically Based and Unified Volumetric Rendering")
        float stepTransmittance = exp(-density * stepSize);
        scattered += transmittance * lightIn * (1.0 - stepTransmittance);
        transmittance *= stepTransmittance;

        if (transmittance < 0.01)
        {
            break;
        }
    }

    return color * transmittance + scattered;
}

// ------------------------------------------------------------------
// Entry points
// ------------------------------------------------------------------

// Fog for a surface at worldPos seen from the camera
float3 ApplyFog(float3 color, float3 worldPos, float2 pixelPos)
{
    float3 toSurface = worldPos - g_CameraPos.xyz;
    float dist = length(toSurface);
    float3 rayDir = toSurface / max(dist, 0.0001);

    if (g_VolumetricFogEnabled != 0)
    {
        color = ApplyVolumetricFog(color, g_CameraPos.xyz, rayDir, dist, pixelPos, true);
    }
    if (g_LinearFogEnabled != 0)
    {
        color = ApplyLinearFog(color, dist);
    }
    return color;
}

// Fog for the skybox: treated as infinitely far away along rayDir
float3 ApplySkyFog(float3 color, float3 rayDir, float2 pixelPos)
{
    if (g_FogApplyToSkybox == 0)
    {
        return color;
    }

    if (g_VolumetricFogEnabled != 0)
    {
        color = ApplyVolumetricFog(color, g_CameraPos.xyz, rayDir, g_FogMaxDistance, pixelPos, true);
    }
    if (g_LinearFogEnabled != 0)
    {
        color = ApplyLinearFog(color, max(g_LinearFogEnd, g_LinearFogStart));
    }
    return color;
}

#endif // FOG_HLSLI
