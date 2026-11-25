#include "Common/math_common.hlsli"

cbuffer MaterialConstants
{
    float4 g_BaseColor; // From source 1
};

cbuffer PBRMaterialConstants
{
    float4 g_BaseColorFactor;
    
    float g_MetallicFactor;
    float g_RoughnessFactor;
    float g_UseBaseColorMap;
    float g_UseMetallicRoughnessMap;
    
    float g_UseNormalMap;
    float g_UseAOMap;
    float g_UseEmissiveMap;
    float g_OverrideFlags; // Bit 1: roughness overridden, bit 2: metallic overridden

    float4 g_EmissiveFactor; // rgb: emissive color * strength (HDR)

    // x: alpha cutoff (0: not a cutout, nothing is discarded), y: index of refraction,
    // z: transmission (0: the surface lets nothing through),
    // w: 1 when the base color's alpha is the opacity of glass, 0 when it means nothing (see SurfaceOpacity)
    float4 g_MaterialParams;

    float4 g_LayerParams;    // x: clear coat strength (0: no coat), y: its roughness, z: sheen roughness, w: specular anti-aliasing strength (0: off)
    float4 g_SheenColor;     // rgb: sheen color (black: no sheen)
};

// Applies the metallic/roughness texture, skipping channels overridden by the model component
void ApplyMetallicRoughnessMap(float4 mrSample, inout float metallic, inout float roughness)
{
    uint flags = (uint)g_OverrideFlags;
    if ((flags & 1u) == 0u) roughness *= mrSample.g;
    if ((flags & 2u) == 0u) metallic *= mrSample.b;
}

// Cutout materials (fences, leaves) discard pixels whose alpha is below their cutoff. Every other material keeps
// them: opaque ones ignore alpha, and blended ones are drawn by the glass pass, which uses it as their opacity.
bool PassesAlphaTest(float alpha)
{
    return g_MaterialParams.x <= 0.0 || alpha >= g_MaterialParams.x;
}

// Define a matching sampler for every texture for Vulkan compatibility
Texture2D g_BaseColorMap;
SamplerState g_BaseColorMap_sampler;

Texture2D g_MetallicRoughnessMap;
SamplerState g_MetallicRoughnessMap_sampler;

Texture2D g_NormalMap;
SamplerState g_NormalMap_sampler;

Texture2D g_AOMap;
SamplerState g_AOMap_sampler;

Texture2D g_EmissiveMap;
SamplerState g_EmissiveMap_sampler;

// Light emitted by the surface: the material's emissive color * strength, masked by the emissive map when there is one
float3 SampleEmissive(float2 uv)
{
    float3 emissive = g_EmissiveFactor.rgb;
    if (g_UseEmissiveMap > 0.5)
    {
        emissive *= g_EmissiveMap.Sample(g_EmissiveMap_sampler, uv).rgb;
    }
    return emissive;
}

// --- Cook-Torrance BRDF Helper Functions ---

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 0.000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / max(denom, 0.000001);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Split-sum environment BRDF, analytic fit (Karis, "Physically Based Shading on Mobile")
// Returns (scale, bias) so that the specular response is F0 * x + y
float2 EnvBRDFApprox(float NdotV, float roughness)
{
    const float4 c0 = float4(-1.0, -0.0275, -0.572, 0.022);
    const float4 c1 = float4(1.0, 0.0425, 1.04, -0.04);
    float4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    return float2(-1.04, 1.04) * a004 + r.zw;
}

// GGX distribution from alpha (the roughness squared), so that a light's size can widen it
float DistributionGGXAlpha(float NdotH, float a)
{
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    // The divisor is only kept off 0 here. Clamping it at 1e-6 flattened the top of every lobe narrower than roughness 0.15
    // (at NdotH 1 it is PI * a2^2, which is 1e-10 for roughness 0.05): the centre of a smooth surface's highlight came out dark.
    return a2 / max(PI * denom * denom, 1e-20);
}

// What one light does to a surface: the diffuse part from the irradiance the light delivers (cosine included), the
// highlight from its radiance as seen along Ls, where the highlight comes from (L for a point of light; for a light
// with a body, the part of it the highlight shows, see RoundLightHighlightDirection in light_shading.hlsli).
// specularRadiance is what the light delivers to a surface facing it; lightRadiance is the radiance of the light's own
// surface (a point of light has none to speak of).
// highlightWidth is the tangent of the angle the body spans seen from the surface; 0 is a point of light. A body
// widens the highlight, and the highlight can show no more than the light's own surface reflects. Karis's widening
// ("Real Shading in Unreal Engine 4", lobe widened by half the angle, energy kept by (a / a')^2) is too dim on a smooth
// surface under a big light: against a numerical integral of the BRDF over a panel and a sphere it was 3 times too
// dark on average, and 70 times for a mirror-like floor under a panel. Here the lobe widens by a quarter of the angle,
// keeps its energy linearly, and the result is capped by F * lightRadiance along the narrow lobe's profile, which is
// the reflection of the light's surface itself; that fits the integral to within 1.35 times on average.
// specularScale lets callers apply multi-scattering energy compensation to the specular lobe.
float3 ShadeLightSource(float3 N, float3 V, float3 Ls, float highlightWidth, float3 diffuseIrradiance, float3 specularRadiance,
    float3 lightRadiance, float3 albedo, float metallic, float roughness, float3 F0, float shadowFactor, float3 specularScale)
{
    float NdotLs = max(dot(N, Ls), 0.0);
    float3 H = normalize(V + Ls);
    float NdotH = max(dot(N, H), 0.0);

    float a = roughness * roughness;
    float widened = saturate(a + 0.25 * highlightWidth);
    float energyKept = a / max(widened, 0.0001);
    float NDF = DistributionGGXAlpha(NdotH, widened) * energyKept;
    float G = GeometrySmith(N, V, Ls, roughness);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotLs + 0.0001;
    float3 specular = numerator / denominator * specularScale * specularRadiance * NdotLs;

    if (highlightWidth > 0.0)
    {
        // The narrow lobe's profile: 1 where the reflected ray lands on the light, falling off past its edge
        float profile = DistributionGGXAlpha(NdotH, a) * PI * a * a;
        specular = min(specular, F * lightRadiance * (profile * specularScale));
    }

    float3 kD = (float3(1.0, 1.0, 1.0) - F) * (1.0 - metallic);
    return (kD * albedo / PI * diffuseIrradiance + specular) * shadowFactor;
}

// ------------------------------------------------------------------
// Layers over the base material: a clear coat and a sheen of cloth fibres
// ------------------------------------------------------------------
// A clear coat is a second, smooth dielectric surface (F0 0.04, the same GGX lobe) over the base layer. It adds its own
// highlight and takes away what it reflects from the layers below. A sheen is a lobe of its own (Charlie distribution,
// Neubelt visibility: Estevez and Kulla, "Production Friendly Microfacet Sheen BRDF", 2017) that brightens a surface
// where it is seen at a slant, like the fibres of cloth. The layers come from the material's constants (ReadMaterialLayers)
// or from the G-buffer; a surface without any has a zero clear coat and a black sheen, which the lighting skips.

struct SurfaceLayers
{
    float Clearcoat;            // Strength, 0: no coat
    float ClearcoatRoughness;
    float3 CoatNormal;          // The coat follows the surface itself, not the base layer's normal map
    float3 SheenColor;          // Black: no sheen
    float SheenRoughness;
};

SurfaceLayers NoSurfaceLayers()
{
    SurfaceLayers layers = (SurfaceLayers) 0;
    layers.CoatNormal = float3(0.0, 0.0, 1.0);
    layers.SheenRoughness = 0.5;
    return layers;
}

SurfaceLayers MakeSurfaceLayers(float clearcoat, float clearcoatRoughness, float3 coatNormal, float3 sheenColor, float sheenRoughness)
{
    SurfaceLayers layers;
    layers.Clearcoat = clearcoat;
    layers.ClearcoatRoughness = max(clearcoatRoughness, 0.05); // As for the base layer: a perfect mirror has no highlight to speak of
    layers.CoatNormal = coatNormal;
    layers.SheenColor = sheenColor;
    layers.SheenRoughness = max(sheenRoughness, 0.1);          // Below this the lobe is so narrow it only flickers
    return layers;
}

bool HasClearcoat(SurfaceLayers layers)
{
    return layers.Clearcoat > 0.003;
}

bool HasSheen(SurfaceLayers layers)
{
    return max(layers.SheenColor.r, max(layers.SheenColor.g, layers.SheenColor.b)) > 0.003;
}

bool HasLayers(SurfaceLayers layers)
{
    return HasClearcoat(layers) || HasSheen(layers);
}

// The layers of the material being drawn. coatNormal: the surface's own normal (the interpolated vertex normal).
SurfaceLayers ReadMaterialLayers(float3 coatNormal)
{
    return MakeSurfaceLayers(g_LayerParams.x, g_LayerParams.y, coatNormal, g_SheenColor.rgb, g_LayerParams.z);
}

float MaxComponent(float3 v)
{
    return max(v.x, max(v.y, v.z));
}

// Sheen distribution: Charlie, a lobe that is widest at the rim of the surface (NdotH near 0)
float DistributionCharlie(float NdotH, float sheenRoughness)
{
    float alpha = sheenRoughness * sheenRoughness;
    float invAlpha = 1.0 / alpha;
    float sin2h = max(1.0 - NdotH * NdotH, 0.0078125);
    return (2.0 + invAlpha) * pow(sin2h, 0.5 * invAlpha) / (2.0 * PI);
}

// Sheen visibility: Neubelt and Pettineo, "Crafting a Next-Gen Material Pipeline for The Order: 1886"
float VisibilityNeubelt(float NdotV, float NdotL)
{
    return 1.0 / (4.0 * max(NdotL + NdotV - NdotL * NdotV, 0.0001));
}

// The share of the light a sheen layer gives back to a viewer at angle NdotV, when it is lit from every direction alike.
// A fit (mean error 0.02) to a numerical integral of the lobe above, which reaches 1 at the rim of a smooth sheen and
// stays under 0.3 face on. Scales what is below the sheen down, and sizes the sheen's reflection of the sky.
float SheenAlbedo(float NdotV, float sheenRoughness)
{
    float r = clamp(sheenRoughness, 0.1, 1.0);
    float A = 2.1051 - 3.7039 * r + 2.0472 * r * r;
    float B = max(8.3841 - 18.0549 * r + 12.0625 * r * r, 0.2);
    float C = -0.0585 + 0.2836 * r - 0.0517 * r * r;
    return saturate(A * pow(saturate(1.0 - NdotV), B) + C);
}

// The light a sheen adds for one light. irradiance: what the light delivers to the surface, cosine included.
// baseScale: how much of what is below the sheen is left (1 - what the sheen takes).
float3 SheenLight(float3 N, float3 V, float3 L, float3 irradiance, SurfaceLayers layers, float shadowFactor, out float baseScale)
{
    float NdotV = max(dot(N, V), 0.0001);
    float NdotL = max(dot(N, L), 0.0);
    float3 H = normalize(V + L);
    float NdotH = saturate(dot(N, H));

    float peak = MaxComponent(layers.SheenColor);
    baseScale = min(1.0 - peak * SheenAlbedo(NdotV, layers.SheenRoughness), 1.0 - peak * SheenAlbedo(max(NdotL, 0.0001), layers.SheenRoughness));

    float lobe = DistributionCharlie(NdotH, layers.SheenRoughness) * VisibilityNeubelt(NdotV, max(NdotL, 0.0001));
    return layers.SheenColor * lobe * irradiance * shadowFactor;
}

// The light a clear coat adds for one light, whose highlight comes from Lc (see RoundLightHighlightDirection; L for a
// point of light). Same inputs as ShadeLightSource, whose lobe it uses for the coat's own surface.
// baseScale: how much of what is below the coat is left.
float3 ClearcoatLight(float3 V, float3 Lc, float highlightWidth, float3 specularRadiance, float3 lightRadiance, SurfaceLayers layers,
    float shadowFactor, out float baseScale)
{
    float3 H = normalize(V + Lc);
    float fresnel = 0.04 + 0.96 * pow(saturate(1.0 - dot(H, V)), 5.0);
    baseScale = 1.0 - layers.Clearcoat * fresnel;

    float3 coat = ShadeLightSource(layers.CoatNormal, V, Lc, highlightWidth, float3(0.0, 0.0, 0.0), specularRadiance, lightRadiance,
        float3(0.0, 0.0, 0.0), 0.0, layers.ClearcoatRoughness, float3(0.04, 0.04, 0.04), shadowFactor, float3(1.0, 1.0, 1.0));
    return coat * layers.Clearcoat;
}

// A light that is a point: radiance is what it delivers to a surface facing it
float3 ComputeLightRadiance(float3 N, float3 V, float3 L, float3 radiance, float3 albedo, float metallic, float roughness, float3 F0, float shadowFactor, float3 specularScale)
{
    return ShadeLightSource(N, V, L, 0.0, radiance * max(dot(N, L), 0.0), radiance, radiance, albedo, metallic, roughness, F0, shadowFactor, specularScale);
}

float3 ComputeLightRadiance(float3 N, float3 V, float3 L, float3 radiance, float3 albedo, float metallic, float roughness, float3 F0, float shadowFactor)
{
    return ComputeLightRadiance(N, V, L, radiance, albedo, metallic, roughness, F0, shadowFactor, float3(1.0, 1.0, 1.0));
}