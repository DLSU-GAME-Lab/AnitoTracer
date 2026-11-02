// hybrid_ps.hlsl
#include "common_struct.hlsli"
#include "light_struct.hlsli"
#include "pbr_defs.hlsli"
#include "fog.hlsli"

// Master, we bind our Scene Acceleration Structure for Ray Queries here!
RaytracingAccelerationStructure g_TLAS;

// Skybox cubemap, used as the image-based light source (black 1x1 fallback when no skybox is loaded)
TextureCube g_EnvMap;
SamplerState g_EnvMap_sampler;

// ------------------------------------------------------------------
// Ray budget / quality knobs
// ------------------------------------------------------------------
static const int SHADOW_SAMPLES = 4;            // Rays per light, spread over the light's area
static const float SUN_ANGULAR_RADIUS = 0.01;   // Radians. Real sun is ~0.0047, slightly larger reads better without denoising
static const float POINT_LIGHT_RADIUS = 0.1;    // World units, gives point lights a soft penumbra
static const int AO_SAMPLES = 6;                // Cosine-weighted occlusion rays
static const float AO_RADIUS = 1.0;             // World units
static const float REFLECTION_MAX_ROUGHNESS = 0.6; // Above this, specular occlusion from AO is enough
static const float REFLECTION_RAY_DISTANCE = 100.0;

// ------------------------------------------------------------------
// Sampling helpers
// ------------------------------------------------------------------

// Point i of n on a unit disk, evenly spread, rotated by phi (radians)
float2 VogelDisk(int i, int n, float phi)
{
    float r = sqrt((i + 0.5) / n);
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
// ------------------------------------------------------------------
float TraceShadowRay(float3 worldPos, float3 normal, float3 lightDir, float maxDist)
{
    // Master! We offset the ray origin along the unperturbed geometric normal to prevent self-shadowing~♡
    RayDesc ray;
    ray.Origin = worldPos + (normal * g_ShadowBias);
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

// Soft shadow from a directional light treated as a disk of SUN_ANGULAR_RADIUS
float TraceDirectionalShadow(float3 worldPos, float3 geoN, float3 L, float noise)
{
    float3 t, b;
    BuildBasis(L, t, b);
    float spread = tan(SUN_ANGULAR_RADIUS);

    float visibility = 0.0;
    for (int s = 0; s < SHADOW_SAMPLES; ++s)
    {
        float2 d = VogelDisk(s, SHADOW_SAMPLES, noise * 2.0 * PI) * spread;
        visibility += TraceShadowRay(worldPos, geoN, normalize(L + t * d.x + b * d.y), 1000.0);
    }
    return visibility / SHADOW_SAMPLES;
}

// Soft shadow from a point light treated as a disk of POINT_LIGHT_RADIUS facing the surface
float TracePointShadow(float3 worldPos, float3 geoN, float3 lightPos, float3 L, float noise)
{
    float3 t, b;
    BuildBasis(L, t, b);

    float visibility = 0.0;
    for (int s = 0; s < SHADOW_SAMPLES; ++s)
    {
        float2 d = VogelDisk(s, SHADOW_SAMPLES, noise * 2.0 * PI) * POINT_LIGHT_RADIUS;
        float3 toSample = lightPos + t * d.x + b * d.y - worldPos;
        float dist = length(toSample);
        visibility += TraceShadowRay(worldPos, geoN, toSample / dist, dist - 0.01);
    }
    return visibility / SHADOW_SAMPLES;
}

// Ray-traced ambient occlusion. Also returns the bent normal (average unoccluded direction)
float TraceAmbientOcclusion(float3 worldPos, float3 geoN, float3 N, float noise, out float3 bentNormal)
{
    float3 t, b;
    BuildBasis(N, t, b);

    float visibility = 0.0;
    float3 openDir = float3(0.0, 0.0, 0.0);
    for (int s = 0; s < AO_SAMPLES; ++s)
    {
        // Disk -> hemisphere projection gives a cosine-weighted distribution (Malley's method)
        float2 d = VogelDisk(s, AO_SAMPLES, noise * 2.0 * PI);
        float3 dir = t * d.x + b * d.y + N * sqrt(saturate(1.0 - dot(d, d)));

        // Normal maps can tilt samples below the real surface; mirror them back above it
        float below = dot(dir, geoN);
        if (below < 0.0)
        {
            dir -= 2.0 * below * geoN;
        }

        float v = TraceShadowRay(worldPos, geoN, dir, AO_RADIUS);
        visibility += v;
        openDir += dir * v;
    }

    bentNormal = dot(openDir, openDir) > 0.0001 ? normalize(openDir) : N;
    return visibility / AO_SAMPLES;
}

// Specular occlusion from ambient occlusion (Lagarde & de Rousiers, "Moving Frostbite to PBR")
float SpecularOcclusion(float NdotV, float ao, float roughness)
{
    return saturate(pow(NdotV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao);
}

// ------------------------------------------------------------------
// Image based lighting from the skybox
// ------------------------------------------------------------------
float GetEnvMipCount()
{
    uint width, height, mips;
    g_EnvMap.GetDimensions(0, width, height, mips);
    return (float)mips;
}

// Cheap irradiance: a few taps of a blurry mip around the normal
float3 SampleEnvIrradiance(float3 N, float mipCount)
{
    float mip = max(mipCount - 3.0, 0.0);

    float3 t, b;
    BuildBasis(N, t, b);

    float3 irradiance = g_EnvMap.SampleLevel(g_EnvMap_sampler, N, mip).rgb * 2.0;
    irradiance += g_EnvMap.SampleLevel(g_EnvMap_sampler, normalize(N + t), mip).rgb;
    irradiance += g_EnvMap.SampleLevel(g_EnvMap_sampler, normalize(N - t), mip).rgb;
    irradiance += g_EnvMap.SampleLevel(g_EnvMap_sampler, normalize(N + b), mip).rgb;
    irradiance += g_EnvMap.SampleLevel(g_EnvMap_sampler, normalize(N - b), mip).rgb;
    return irradiance / 6.0;
}

// Rougher surfaces read blurrier mips
float3 SampleEnvSpecular(float3 R, float roughness, float mipCount)
{
    float mip = roughness * max(mipCount - 1.0, 0.0);
    return g_EnvMap.SampleLevel(g_EnvMap_sampler, R, mip).rgb;
}

void main_ps(in PSInput In, out float4 OutColor : SV_TARGET)
{
    // 1. Material Sampling
    float4 albedo = g_BaseColorFactor;
    if (g_UseBaseColorMap > 0.5)
    {
        albedo *= g_BaseColorMap.Sample(g_BaseColorMap_sampler, In.UV);
    }

    // Alpha testing for discarded fragments
    if (albedo.a < 0.5)
    {
        discard;
    }

    float metallic = g_MetallicFactor;
    float roughness = g_RoughnessFactor;
    if (g_UseMetallicRoughnessMap > 0.5)
    {
        float4 mrSample = g_MetallicRoughnessMap.Sample(g_MetallicRoughnessMap_sampler, In.UV);
        ApplyMetallicRoughnessMap(mrSample, metallic, roughness);
    }
    roughness = max(roughness, 0.05);

    float ao = 1.0;
    if (g_UseAOMap > 0.5)
    {
        ao = g_AOMap.Sample(g_AOMap_sampler, In.UV).r;
    }

    // Unperturbed geometric normal used strictly for ray origin offsets!
    float3 geoN = normalize(In.Normal);

    float3 N = geoN;
    float3 T = normalize(In.Tangent);
    float3 B = normalize(In.Bitangent);

    float3x3 TBN = float3x3(T, B, N);

    // Normal mapping
    if (g_UseNormalMap > 0.5)
    {
        float3 tangentNormal = g_NormalMap.Sample(g_NormalMap_sampler, In.UV).xyz * 2.0 - 1.0;
        N = normalize(mul(tangentNormal, TBN));
    }

    float3 V = normalize(g_CameraPos.xyz - In.WorldPos);
    float NdotV = saturate(dot(N, V)) + 0.0001;
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, metallic);
    float3 Lo = float3(0.0, 0.0, 0.0);

    // Per-pixel rotation for all stochastic rays, so the sample patterns don't band
    float noise = InterleavedGradientNoise(In.Pos.xy);

    // Single scattering GGX loses energy on rough surfaces; scale it back up (Fdez-Aguera 2019)
    float2 envBRDF = EnvBRDFApprox(NdotV, roughness);
    float3 specularColor = F0 * envBRDF.x + envBRDF.y;
    float3 energyCompensation = 1.0 + F0 * (1.0 / max(envBRDF.x + envBRDF.y, 0.0001) - 1.0);

    // 2. Directional Lights (Using our common PBR BRDF helper!)
    for (int i = 0; i < g_NumDirLights; ++i)
    {
        float3 L = normalize(-g_DirLights[i].Direction.xyz);
        if (dot(N, L) <= 0.0)
            continue;

        float shadowFactor = TraceDirectionalShadow(In.WorldPos, geoN, L, noise);
        float3 radiance = g_DirLights[i].Color.rgb * g_DirLights[i].Color.a;

        Lo += ComputeLightRadiance(N, V, L, radiance, albedo.rgb, metallic, roughness, F0, shadowFactor, energyCompensation);
    }

    // 3. Point Lights
    for (int j = 0; j < g_NumPointLights; ++j)
    {
        float3 lightVec = g_PointLights[j].Position.xyz - In.WorldPos;
        float dist = length(lightVec);

        if (dist < g_PointLights[j].Range)
        {
            float3 L = lightVec / dist;
            if (dot(N, L) <= 0.0)
                continue;

            float shadowFactor = TracePointShadow(In.WorldPos, geoN, g_PointLights[j].Position.xyz, L, noise);

            float attenuation = max(1.0 - (dist / g_PointLights[j].Range), 0.0);
            attenuation *= attenuation;
            float3 radiance = g_PointLights[j].Color.rgb * g_PointLights[j].Color.a * attenuation;

            Lo += ComputeLightRadiance(N, V, L, radiance, albedo.rgb, metallic, roughness, F0, shadowFactor, energyCompensation);
        }
    }

    // Emissive Sampling
    float3 emissive = float3(0.0, 0.0, 0.0);
    if (g_UseEmissiveMap > 0.5)
    {
        emissive = g_EmissiveMap.Sample(g_EmissiveMap_sampler, In.UV).rgb;
    }

    // 4. Ambient: flat ambient color plus skybox light, both occluded by ray-traced AO
    float3 bentNormal;
    float rtao = TraceAmbientOcclusion(In.WorldPos, geoN, N, noise, bentNormal);
    float occlusion = min(ao, rtao);

    float envMipCount = GetEnvMipCount();
    float3 irradiance = g_AmbientColor.rgb + SampleEnvIrradiance(bentNormal, envMipCount) * g_SkyLightIntensity;
    float3 kD = (1.0 - specularColor) * (1.0 - metallic);
    float3 ambientDiffuse = irradiance * albedo.rgb * kD * occlusion;

    // 5. Reflections of the skybox, blocked where scene geometry is in the way
    float3 R = reflect(-V, N);
    float specOcclusion = SpecularOcclusion(NdotV, occlusion, roughness);

    // Normal maps can point reflections into the surface; fade those out
    float horizon = saturate(1.0 + dot(R, geoN));
    specOcclusion *= horizon * horizon;

    float3 envSpecular = SampleEnvSpecular(R, roughness, envMipCount) * g_SkyLightIntensity;
    if (roughness < REFLECTION_MAX_ROUGHNESS && g_ReflectionIntensity > 0.0)
    {
        // A reflection ray can't shade what it hits (no hit shading here), so stand in a dim
        // bounce of the local irradiance for the occluder instead of showing sky through it
        float reflectionVisibility = TraceShadowRay(In.WorldPos, geoN, R, REFLECTION_RAY_DISTANCE);
        float3 occludedRadiance = irradiance * 0.25 * occlusion;
        float traceWeight = 1.0 - smoothstep(REFLECTION_MAX_ROUGHNESS * 0.5, REFLECTION_MAX_ROUGHNESS, roughness);
        envSpecular = lerp(envSpecular, lerp(occludedRadiance, envSpecular, reflectionVisibility), traceWeight);
    }
    float3 ambientSpecular = envSpecular * specularColor * energyCompensation * specOcclusion * g_ReflectionIntensity;

    float3 ambient = (ambientDiffuse + ambientSpecular) * g_AmbientMultiplier;
    float3 color = ambient + Lo + emissive;
    color = ApplyFog(color, In.WorldPos, In.Pos.xy);

    OutColor = float4(color, albedo.a);
}
