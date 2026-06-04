#version 460
#extension GL_EXT_nonuniform_qualifier : require

#define MAX_SHADOW_LIGHTS 4

// ── Uniform buffer: camera matrices ─────────────────────────────────────────
// Layout MUST match Assets::UniformBufferObject in UniformBuffer.hpp exactly.
layout(binding = 0) uniform UniformBufferObject
{
	mat4  ModelView;
	mat4  Projection;
	mat4  ModelViewInverse;
	mat4  ProjectionInverse;
	float Aperture;
	float FocusDistance;
	float HeatmapScale;
	uint  TotalNumberOfSamples;
	uint  NumberOfSamples;
	uint  SamplesPerInvocation;
	uint  NumberOfBounces;
	uint  RandomSeed;
	uint  MaxRays;
	uint  HasSky;
	uint  ShowHeatmap;
	uint  EnableAdaptiveSampling;
	float VarianceThreshold;
	uint  MinSamples;
	vec3  FallbackAmbientColor; // RGB fallback ambient when IBL is disabled
	float Exposure;             // Scene exposure scalar applied to direct lighting before tonemapping
	uint  UseColorIBL;          // When non-zero, IBLSkyColor replaces cubemap sampling
	// std140 implicit padding (12 B) to align IBLSkyColor vec3 to 16-byte boundary
	vec3  IBLSkyColor;          // Flat sky tint used when UseColorIBL != 0
	// std140 implicit trailing padding (4 B)
} ubo;

// ── Material buffer (matches Assets::Material alignas(16)) ───────────────────
struct Material
{
	vec4  Diffuse;           // .rgb = albedo, .a = 1
	int   DiffuseTextureId;  // -1 means no texture
	float Fuzziness;         // Metallic fuzziness / roughness
	float RefractionIndex;
	uint  MaterialModel;     // 0=Lambertian 1=Metallic 2=Dielectric 3=Isotropic 4=DiffuseLight
};
layout(binding = 1) readonly buffer MaterialBuffer { Material materials[]; };

// ── Light buffer ─────────────────────────────────────────────────────────────
struct LightProperties
{
	// Layout MUST match Assets::LightProperties (alignas(16) vec3 → vec4 slots)
	vec4  LightPos;      // .xyz = position,  .w = unused
	vec4  LightDir;      // .xyz = direction, .w = unused
	vec4  AmbientColor;  // .rgba (unused in PBR pass, kept for layout parity)
	vec4  LightColor;    // .rgb = color,     .a = intensity
	uint  LightType;     // 0=Point 1=Directional 2=Spot
	float _pad0; float _pad1; float _pad2;
};
layout(binding = 2) readonly buffer LightBuffer { LightProperties lights[]; };

// ── Texture array ─────────────────────────────────────────────────────────────
layout(binding = 3) uniform sampler2D textures[];

// ── Skybox cubemap ────────────────────────────────────────────────────────────
layout(binding = 4) uniform samplerCube skybox;

// ── Shadow maps (binding 5): one sampler2DShadow per active shadow light ──────
// Hardware PCF compare (LESS_OR_EQUAL).  texture() returns 1.0=lit, 0.0=shadow.
layout(binding = 5) uniform sampler2DShadow shadowMaps[MAX_SHADOW_LIGHTS];

// ── Shadow UBO (binding 6): all directional-light VP matrices + active count ──
layout(binding = 6) uniform ShadowUBO
{
	mat4 LightViewProj[MAX_SHADOW_LIGHTS];
	uint Count;
} shadowUBO;

// ── Point light shadow maps (binding 7): cubemap shadows with PCF compare ─────
#define MAX_POINT_SHADOW_LIGHTS 4
layout(binding = 7) uniform samplerCubeShadow pointShadowMaps[MAX_POINT_SHADOW_LIGHTS];

// ── Point shadow UBO (binding 8): cubemap VP matrices + light positions + count
layout(binding = 8) uniform PointShadowUBO
{
	mat4  CubemapViewProj[MAX_POINT_SHADOW_LIGHTS * 6];  // 6 VP matrices per light
	vec4  LightPositions[MAX_POINT_SHADOW_LIGHTS];       // Light positions (world-space)
	uint  Count;                                          // Number of active point lights
	float FarPlane;                                       // Far plane used for linear depth encoding
	float _pad[2];
} pointShadowUBO;

// ── IBL textures (binding 9/10/11) ───────────────────────────────────────────
// Bound by GameRenderer to IBLPrecompute outputs when a skybox is present.
// When IBL is unavailable (no skybox) these are dummy-bound to the skybox
// sampler; the shader guards access with (ubo.HasSky != 0).
layout(binding = 9)  uniform samplerCube iblIrradiance;    // 32×32 diffuse irradiance
layout(binding = 10) uniform samplerCube iblPrefiltered;   // 128×128 specular (mipped)
layout(binding = 11) uniform sampler2D   brdfLUT;          // 512×512 split-sum LUT

// ========== NEW: Normal mapping and PBR texture support ==========

// Struct matching Vulkan::Game::GameRendererMaterialPropertiesAlphaCutoff
// Layout MUST match the C++ header exactly (std140 alignment, 64 bytes total)
struct GameRenderMaterialProperties
{
	// ===== Original Properties (first 48 bytes) =====
	int   NormalMapTextureId;
	float NormalMapStrength;
	int   MetallicMapTextureId;
	float MetallicValue;
	int   RoughnessMapTextureId;
	float RoughnessValue;
	int   AOMapTextureId;
	float AOStrength;

	// ===== NEW: Alpha Cutoff Properties (second 16 bytes) =====
	int   AlphaMapTextureId;           // -1 = no alpha map
	float AlphaCutoffThreshold;        // Alpha test threshold (0.0-1.0)
	uint  AlphaBlendMode;              // 0=Opaque, 1=Transparent, 2=Additive
	float _pad;                        // Padding for alignment

	// Note: Additional padding (_padEnd[3]) is part of the buffer's 64-byte boundary,
	// but GLSL std140 handles this automatically; no explicit declaration needed here.
};

// Binding 12: Normal maps texture array (only exists when scene has textures)
layout(binding = 12) uniform sampler2D normalMaps[];

// Binding 13: Material properties buffer (only exists when scene has textures)
layout(binding = 13) readonly buffer GameRenderMatProps
{
	GameRenderMaterialProperties materialProperties[];
};

// Binding 14: Metallic maps texture array (only exists when scene has textures)
layout(binding = 14) uniform sampler2D metallicMaps[];

// Binding 15: Roughness maps texture array (only exists when scene has textures)
layout(binding = 15) uniform sampler2D roughnessMaps[];

// Binding 16: Ambient occlusion maps texture array (only exists when scene has textures)
layout(binding = 16) uniform sampler2D aoMaps[];

// Binding 17: Alpha channel texture maps (for transparency/alpha test support)
layout(binding = 17) uniform sampler2D alphaMaps[];

// ========== Helper Functions for Normal Mapping ==========

/// Constructs orthonormal TBN matrix from tangent and normal
/// Note: After interpolation, tangent and normal may not be perfectly orthogonal.
/// We reconstruct bitangent via cross product (inherently perpendicular) rather
/// than Gram-Schmidt, which can be numerically unstable on normalized vectors.
mat3 ConstructTBNMatrix(vec3 normal, vec3 tangent)
{
	// Normalize in case interpolation scaled the vectors
	normal = normalize(normal);
	tangent = normalize(tangent);

	// Cross product is ALWAYS perpendicular — numerically stable and reliable
	vec3 bitangent = cross(normal, tangent);

	// Ensure bitangent is normalized (cross of two unit vectors may not be exactly unit)
	bitangent = normalize(bitangent);

	return mat3(tangent, bitangent, normal);
}

/// Samples normal map and applies it to the surface normal
/// When no textures exist (texCount == 0), this gracefully returns the input normal
vec3 SampleAndApplyNormalMap(
	int materialIndex, 
	vec3 normal, 
	vec3 tangent, 
	vec2 texCoord)
{
	GameRenderMaterialProperties matProps = materialProperties[materialIndex];

	// If no normal map, return the interpolated normal
	if (matProps.NormalMapTextureId < 0)
		return normal;

	// Sample normal map (expected to be in tangent space, where RGB = XYZ tangent coords)
	vec3 sampledNormal = texture(normalMaps[nonuniformEXT(matProps.NormalMapTextureId)], texCoord).rgb;

	// Convert from [0,1] range to [-1,1] range
	sampledNormal = normalize(sampledNormal * 2.0 - 1.0);

	// Apply normal map strength: blend between flat (0,0,1) and sampled normal
	sampledNormal = normalize(mix(vec3(0.0, 0.0, 1.0), sampledNormal, matProps.NormalMapStrength));

	// Construct TBN matrix and transform sampled normal to world space
	mat3 tbnMatrix = ConstructTBNMatrix(normal, tangent);
	vec3 worldNormal = normalize(tbnMatrix * sampledNormal);

	return worldNormal;
}

/// Samples metallic map if available, otherwise returns material default
/// When textures don't exist, gracefully returns the default value
float SampleMetallicMap(int materialIndex, float defaultMetallic, vec2 texCoord)
{
	GameRenderMaterialProperties matProps = materialProperties[materialIndex];

	if (matProps.MetallicMapTextureId < 0)
		return defaultMetallic;

	// Sample metallic map (typically uses R channel)
	float sampledMetallic = texture(metallicMaps[nonuniformEXT(matProps.MetallicMapTextureId)], texCoord).r;

	// Blend material metallic value with sampled value
	return mix(matProps.MetallicValue, sampledMetallic, 1.0);
}

/// Samples roughness map if available, otherwise returns material default
/// When textures don't exist, gracefully returns the default value
float SampleRoughnessMap(int materialIndex, float defaultRoughness, vec2 texCoord)
{
	GameRenderMaterialProperties matProps = materialProperties[materialIndex];

	if (matProps.RoughnessMapTextureId < 0)
		return defaultRoughness;

	// Sample roughness map (typically uses R channel)
	float sampledRoughness = texture(roughnessMaps[nonuniformEXT(matProps.RoughnessMapTextureId)], texCoord).r;

	// Blend material roughness value with sampled value
	return mix(matProps.RoughnessValue, sampledRoughness, 1.0);
}

/// Samples AO map if available, otherwise returns 1.0 (no occlusion)
/// When textures don't exist, gracefully returns 1.0 (full brightness)
float SampleAOMap(int materialIndex, vec2 texCoord)
{
	GameRenderMaterialProperties matProps = materialProperties[materialIndex];

	if (matProps.AOMapTextureId < 0)
		return 1.0;  // No occlusion when no map available

	// Sample AO map (typically uses R channel for grayscale occlusion)
	float sampledAO = texture(aoMaps[nonuniformEXT(matProps.AOMapTextureId)], texCoord).r;

	// Apply AO strength: blend between full brightness (1.0) and sampled value
	return mix(1.0, sampledAO, matProps.AOStrength);
}

// ========== Helper Functions for Alpha Cutoff & Transparency ==========

/// @brief Samples alpha from the appropriate texture or material channel.
/// 
/// Priority order:
///   1. If AlphaMapTextureId >= 0: use that texture (R channel, or RGBA avg)
///   2. Else if DiffuseTextureId >= 0: use diffuse texture alpha channel
///   3. Else: return 1.0 (fully opaque)
///
/// @param materialIndex Index into materialProperties buffer
/// @param mat           Material from materials buffer
/// @param texCoord      UV coordinates for texture sampling
/// @return Alpha value in [0, 1] range
float SampleAlpha(int materialIndex, in Material mat, vec2 texCoord)
{
	GameRenderMaterialProperties matProps = materialProperties[materialIndex];

	// First priority: dedicated alpha map texture
	if (matProps.AlphaMapTextureId >= 0)
	{
		// Sample dedicated alpha map (typically uses R channel for grayscale)
		float sampledAlpha = texture(alphaMaps[nonuniformEXT(matProps.AlphaMapTextureId)], texCoord).r;
		return sampledAlpha;
	}

	// Second priority: alpha channel from diffuse texture
	if (mat.DiffuseTextureId >= 0)
	{
		// Sample diffuse texture and extract alpha channel
		float diffuseAlpha = texture(textures[nonuniformEXT(mat.DiffuseTextureId)], texCoord).a;
		return diffuseAlpha;
	}

	// Default: fully opaque
	return 1.0;
}

/// @brief Applies alpha cutoff test and handles transparency blending.
///
/// This function should be called early in main() after computing alpha value.
/// 
/// Behavior:
///   - AlphaBlendMode 0 (Opaque):      Discard if alpha < threshold, else set alpha=1.0
///   - AlphaBlendMode 1 (Transparent): Allow partial alpha (no cutoff)
///   - AlphaBlendMode 2 (Additive):    Allow partial alpha (no cutoff), prepare for additive
///   - Unknown mode:                    Treat as Opaque (safest default)
///
/// @param alpha         Sampled alpha value [0, 1]
/// @param materialIndex Index into materialProperties buffer
/// @return Final alpha value to use for output color
float ApplyAlphaCutoff(float alpha, int materialIndex)
{
	GameRenderMaterialProperties matProps = materialProperties[materialIndex];

	uint blendMode = matProps.AlphaBlendMode;

	if (blendMode == 0u) {
		// Opaque mode: apply alpha cutoff (alpha test)
		if (alpha < matProps.AlphaCutoffThreshold) {
			discard;  // Fragment is transparent, remove it
		}
		return 1.0;  // Force fully opaque for opaque materials
	}
	else if (blendMode == 1u) {
		// Transparent mode: preserve alpha for blending
		return alpha;
	}
	else if (blendMode == 2u) {
		// Additive mode: preserve alpha for additive blending
		return alpha;
	}
	else {
		// Unknown/invalid mode: default to opaque for safety
		if (alpha < matProps.AlphaCutoffThreshold) {
			discard;
		}
		return 1.0;
	}
}

/// @brief Convenience function to sample alpha and apply cutoff in one call.
///
/// Recommended usage in main():
///   float finalAlpha = SampleAndApplyAlpha(inMaterialIndex, mat, inTexCoord);
///
/// @param materialIndex Index into materialProperties buffer
/// @param mat           Material from materials buffer
/// @param texCoord      UV coordinates for texture sampling
/// @return Final alpha value after cutoff processing
float SampleAndApplyAlpha(int materialIndex, in Material mat, vec2 texCoord)
{
	float alpha = SampleAlpha(materialIndex, mat, texCoord);
	return ApplyAlphaCutoff(alpha, materialIndex);
}

// Number of roughness mip levels in the prefiltered cubemap
// (must match IBLPrecompute::kPrefilteredMips)
const uint MAX_PREFILTER_MIPS = 7u;

// ── Inputs from vertex shader ─────────────────────────────────────────────────
layout(location = 0) in vec3  inWorldPos;
layout(location = 1) in vec3  inNormal;
layout(location = 2) in vec2  inTexCoord;
layout(location = 3) in flat int inMaterialIndex;
layout(location = 4) in vec3  inTangent;  // NEW: Tangent for TBN matrix

// ── Output ────────────────────────────────────────────────────────────────────
layout(location = 0) out vec4 outColor;

// ─────────────────────────────────────────────────────────────────────────────
// PBR helpers
// ─────────────────────────────────────────────────────────────────────────────
const float PI = 3.14159265359;

float D_GGX(float NdotH, float roughness)
{
	float a  = roughness * roughness;
	float a2 = a * a;
	float d  = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
	return a2 / max(PI * d * d, 0.0001);
}

float G_SmithSchlick(float NdotV, float NdotL, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	float gv = NdotV / max(NdotV * (1.0 - k) + k, 0.0001);
	float gl = NdotL / max(NdotL * (1.0 - k) + k, 0.0001);
	return gv * gl;
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CookTorranceBRDF(vec3 N, vec3 V, vec3 L,
					  vec3 albedo, float metallic, float roughness)
{
	// Light is behind the surface — no contribution, and V+L would be
	// a near-zero vector causing normalize() to produce garbage NdotH values.
	float NdotL = dot(N, L);
	if (NdotL <= 0.0) return vec3(0.0);

	vec3  H     = normalize(V + L);
	float NdotV = max(dot(N, V), 0.0001);
	// Use safe value for BRDF denominator/geometry term only; raw NdotL
	// is used in the final multiply to get the correct cosine falloff.
	float NdotL_safe = max(NdotL, 0.0001);
	float NdotH = max(dot(N, H), 0.0);
	float HdotV = max(dot(H, V), 0.0);

	vec3  F0  = mix(vec3(0.04), albedo, metallic);
	float D   = D_GGX(NdotH, roughness);
	float G   = G_SmithSchlick(NdotV, NdotL_safe, roughness);
	vec3  F   = F_Schlick(HdotV, F0);

	vec3 kD       = (1.0 - F) * (1.0 - metallic);
	vec3 diffuse  = kD * albedo / PI;
	vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL_safe, 0.0001);

	// Multiply by raw NdotL — gives the correct smooth Lambert cosine falloff
	// (bright at the pole, smoothly darkening toward the terminator, zero past it)
	return (diffuse + specular) * NdotL;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tone mapping
// ─────────────────────────────────────────────────────────────────────────────

// ACES filmic tone mapping — preserves hue and colour at extreme HDR values
// much better than Reinhard. Tuned for input in roughly [0, 8].
vec3 ACESToneMapping(vec3 color)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// PCF Shadow — 3×3 kernel (9 samples)
// ─────────────────────────────────────────────────────────────────────────────
// PCF Shadow — 3×3 kernel (9 samples) for one shadow map slot.
//
//   shadowIdx : index into shadowMaps[] and shadowUBO.LightViewProj[].
//   worldPos  : fragment world-space position (from vertex shader).
//
//   Returns 1.0 = fully lit, 0.0 = fully in shadow.
//   Pixels outside the light frustum are treated as fully lit.
// ─────────────────────────────────────────────────────────────────────────────
float SampleShadowPCF(uint shadowIdx, vec3 worldPos)
{
	// Transform world-space position into the light's clip space.
	vec4 shadowCoord = shadowUBO.LightViewProj[shadowIdx] * vec4(worldPos, 1.0);

	// Perspective divide — for an ortho projection w == 1, but kept for correctness.
	vec3 proj = shadowCoord.xyz / shadowCoord.w;

	// Remap X,Y from Vulkan NDC [-1, 1] → UV [0, 1].
	// Z is already in [0, 1] (GLM_FORCE_DEPTH_ZERO_TO_ONE).
	proj.xy = proj.xy * 0.5 + 0.5;

	// Slope-scaled software bias: reduces acne on surfaces at grazing angles
	// to the light without over-biasing thin horizontal surfaces.
	// kBiasMin is the constant term; kBiasSlope scales with how much the
	// fragment normal deviates from the light direction.
	const float kBiasMin   = 0.0005;
	const float kBiasSlope = 0.005;
	const float ref        = proj.z - kBiasMin;
	const vec2  texelSize  = 1.0 / vec2(textureSize(shadowMaps[shadowIdx], 0));

	// Clamp the UV center so the 3×3 PCF kernel cannot stray into the
	// CLAMP_TO_BORDER region (which returns 1.0 = fully lit via the white
	// border).  Without this clamp, fragments right at the frustum edge
	// sample half-lit because several kernel taps land outside the [0,1]
	// range and come back white, creating a bright-lit strip along every
	// geometry edge that happens to sit near the frustum boundary.
	const vec2 halfTexel = texelSize * 1.5; // kernel reaches ±1 texel
	const vec2 uvClamped = clamp(proj.xy, halfTexel, vec2(1.0) - halfTexel);

	float shadow = 0.0;
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			// texture(sampler2DShadow, vec3(uv, ref)) returns
			// 1.0 when ref <= depth_in_shadowmap  (not in shadow)
			// 0.0 when ref >  depth_in_shadowmap  (in shadow)
			shadow += texture(shadowMaps[shadowIdx],
							  vec3(uvClamped + vec2(x, y) * texelSize, ref));
		}
	}
	return shadow / 9.0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Point Light Cubemap Shadow — linear depth compare
// ─────────────────────────────────────────────────────────────────────────────
/// @brief Sample point light shadow using cubemap.
///   pointIdx : index into pointShadowMaps[] and pointShadowUBO
///   worldPos : fragment world-space position
///   Returns 1.0 = fully lit, 0.0 = fully in shadow
float SamplePointLightShadow(uint pointIdx, vec3 worldPos)
{
	vec3  lightPos   = pointShadowUBO.LightPositions[pointIdx].xyz;
	vec3  toFragment = worldPos - lightPos;

	// Linear reference depth — matches what point_shadow_frag.frag writes
	// (dist / FarPlane).  A small world-space bias is subtracted so the
	// surface doesn't shadow itself (avoids self-shadow acne).
	const float kBiasWorld = 1.5;        // world-space units; tune if needed
	float ref = (length(toFragment) - kBiasWorld) / pointShadowUBO.FarPlane;
	ref = max(ref, 0.0);                  // clamp: never negative

	// texture(samplerCubeShadow, vec4(dir, ref)):
	//   dir selects the cubemap face + UV
	//   ref is compared against the stored depth via the sampler compare op
	//   returns 1.0 = lit  (ref < stored depth),  0.0 = in shadow
	return texture(pointShadowMaps[pointIdx], vec4(toFragment, ref));
}

// ─────────────────────────────────────────────────────────────────────────────
// Derive PBR parameters from the Material enum
// ─────────────────────────────────────────────────────────────────────────────
void MaterialToPBR(in Material mat, in vec3 texSample,
				   out vec3 albedo, out float metallic, out float roughness,
				   out bool isEmissive)
{
	albedo    = mat.Diffuse.rgb * texSample;
	metallic  = 0.0;
	roughness = 1.0;
	isEmissive = false;

	// 0 = Lambertian  → pure diffuse
	// 1 = Metallic    → full metal, fuzziness = roughness
	// 2 = Dielectric  → smooth dielectric, tinted
	// 3 = Isotropic   → uniform scatter (treat as diffuse)
	// 4 = DiffuseLight → emissive
	if (mat.MaterialModel == 1u) {
		metallic  = 1.0;
		roughness = clamp(mat.Fuzziness, 0.02, 1.0);
	} else if (mat.MaterialModel == 2u) {
		metallic  = 0.0;
		roughness = 0.05;
	} else if (mat.MaterialModel == 4u) {
		isEmissive = true;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// IBL ambient (diffuse irradiance + specular split-sum)
// ─────────────────────────────────────────────────────────────────────────────
// Returns the image-based ambient contribution in linear HDR space.
// Must be added AFTER tone-mapping of direct light is applied so it lands
// in the same display-space [0,1] range as the FallbackAmbientColor path.
vec3 IBLAmbient(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness)
{
	vec3 F0 = mix(vec3(0.04), albedo, metallic);
	vec3 F  = F_Schlick(max(dot(N, V), 0.0), F0);

	// For a real HDR cubemap the Fresnel term on kD is physically correct —
	// different directions of the environment contribute different energies and
	// the view-dependent Fresnel conserves energy across diffuse + specular.
	//
	// For a flat uniform colour (UseColorIBL=1) the ambient arrives equally
	// from all directions, so there is no directional energy imbalance to
	// compensate for.  Applying the Fresnel here would create an artificial
	// view-angle darkening (kD → 0 at grazing NdotV) that manifests as
	// SSAO-like contact shadows that flip sides when the camera orbits —
	// a purely view-dependent artefact with no physical basis.
	vec3 kD;
	if (ubo.UseColorIBL != 0u)
		kD = vec3(1.0 - metallic);          // uniform ambient — no Fresnel attenuation
	else
		kD = (1.0 - F) * (1.0 - metallic); // real cubemap — physically correct

	// Diffuse: hemisphere-integrated irradiance.
	// When UseColorIBL is set, substitute the flat IBLSkyColor for the cubemap
	// so the tint drives ambient rather than the actual HDR environment capture.
	vec3 irradiance = (ubo.UseColorIBL != 0u)
		? ubo.IBLSkyColor
		: texture(iblIrradiance, N).rgb;
	vec3 diffuseIBL = kD * irradiance * albedo;

	// Specular: GGX-prefiltered env at the matching roughness mip.
	// Same substitution — flat colour acts as a uniform environment for specular.
	vec3 prefilteredEnv;
	vec3 specularIBL;

	if (ubo.UseColorIBL != 0u)
	{
		// For flat colour IBL: use simplified specular calculation
		// without complex BRDF lookup that can cause grainy artifacts
		prefilteredEnv = ubo.IBLSkyColor;
		// Simplified specular: just apply Fresnel to the flat environment colour
		specularIBL = prefilteredEnv * F * (1.0 - roughness * 0.5);  // Reduce spec for non-metallic
	}
	else
	{
		// Real HDR cubemap: full PBR with BRDF integration
		float mipLevel = roughness * float(MAX_PREFILTER_MIPS - 1u);
		prefilteredEnv = textureLod(iblPrefiltered, reflect(-V, N), mipLevel).rgb;
		vec2 brdfSample  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
		specularIBL = prefilteredEnv * (F0 * brdfSample.x + brdfSample.y);
	}

	return diffuseIBL + specularIBL;
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
void main()
{
	Material mat = materials[inMaterialIndex];

	// ===== NEW: Early exit for alpha cutoff/transparency =====
	float finalAlpha = SampleAndApplyAlpha(inMaterialIndex, mat, inTexCoord);
	// If alpha cutoff was applied (Opaque mode), we never reach here for discarded pixels

	// Sample diffuse texture (or white if none)
	vec3 texSample = vec3(1.0);
	if (mat.DiffuseTextureId >= 0)
		texSample = texture(textures[nonuniformEXT(mat.DiffuseTextureId)], inTexCoord).rgb;

	vec3  albedo;
	float metallic, roughness;
	bool  isEmissive;
	MaterialToPBR(mat, texSample, albedo, metallic, roughness, isEmissive);

	// Emissive materials simply output their albedo colour (used by Bloom in later phases)
	if (isEmissive)
	{
		outColor = vec4(albedo, 1.0);
		return;
	}

	vec3 N = normalize(inNormal);
	vec3 V = normalize(-inWorldPos); // View direction (camera at origin in view space)

	// Extract camera world position from the inverse view matrix
	vec3 camWorldPos = vec3(ubo.ModelViewInverse[3]);
	V = normalize(camWorldPos - inWorldPos);

	// ===== NEW: Apply normal map =====
	N = SampleAndApplyNormalMap(inMaterialIndex, N, inTangent, inTexCoord);

	// ===== NEW: Apply texture-based metallic/roughness/AO =====
	metallic = SampleMetallicMap(inMaterialIndex, metallic, inTexCoord);
	roughness = SampleRoughnessMap(inMaterialIndex, roughness, inTexCoord);
	float ao = SampleAOMap(inMaterialIndex, inTexCoord);

	// Accumulate direct lighting in physical / HDR space.
	// Ambient is handled separately because FallbackAmbientColor is a
	// display-space [0,1] value and must not be exposure-scaled.
	vec3 directLight = vec3(0.0);
	uint lightCount  = uint(lights.length());

	// shadowIdx tracks which shadow map slot corresponds to the current
	// directional light — increments once per directional light encountered.
	uint shadowIdx      = 0;
	uint pointShadowIdx = 0;

	for (uint i = 0; i < lightCount; ++i)
	{
		LightProperties light = lights[i];
		vec3  L;
		float attenuation = 1.0;

		if (light.LightType == 0u) {
			// Point light
			vec3  toLight = light.LightPos.xyz - inWorldPos;
			float dist    = length(toLight);
			L             = normalize(toLight);
			attenuation   = 1.0 / max(dist * dist, 0.0001);
		} else if (light.LightType == 1u) {
			// Directional light
			L           = normalize(-light.LightDir.xyz);
			attenuation = 1.0;
		} else {
			// Spot light
			vec3  toLight  = light.LightPos.xyz - inWorldPos;
			float dist     = length(toLight);
			L              = normalize(toLight);
			float theta    = dot(L, normalize(-light.LightDir.xyz));
			float cutoff   = cos(radians(30.0));
			attenuation    = (theta > cutoff) ? (1.0 / max(dist * dist, 0.0001)) : 0.0;
		}

		// Shadow modulation: directional lights use 2D PCF, point lights use cubemap
		float shadowFactor = 1.0;
		if (light.LightType == 1u)
		{
			if (shadowIdx < shadowUBO.Count)
				shadowFactor = SampleShadowPCF(shadowIdx, inWorldPos);
			++shadowIdx;
		}
		else if (light.LightType == 0u)
		{
			if (pointShadowIdx < pointShadowUBO.Count)
				shadowFactor = SamplePointLightShadow(pointShadowIdx, inWorldPos);
			++pointShadowIdx;
		}

		float intensity   = light.LightColor.a;
		vec3  lightColor  = light.LightColor.rgb * intensity;

		directLight += shadowFactor
					 * CookTorranceBRDF(N, V, L, albedo, metallic, roughness)
					 * lightColor * attenuation;
	}

	// ── Ambient / IBL ────────────────────────────────────────────────────────
	// Three paths depending on HasSky and UseColorIBL:
	//
	// A) HasSky=1, UseColorIBL=0  — real HDR cubemap.
	//    Irradiance values are HDR (>> 1), so they must be added to directLight
	//    BEFORE exposure scaling + tonemapping so everything lands on the same curve.
	//
	// B) HasSky=1, UseColorIBL=1  — flat IBLSkyColor in display-space [0,1].
	//    Treating it like HDR would multiply it by Exposure (≈0.00001) and kill it.
	//    Instead, tonemap direct light first, then add the ambient term afterwards —
	//    the same treatment used by FallbackAmbientColor.
	//
	// C) HasSky=0  — no skybox / IBL disabled.
	//    Tonemap direct light, then add FallbackAmbientColor.
	vec3 ambientTerm;
	if (ubo.HasSky != 0u && ubo.UseColorIBL == 0u)
	{
		// Path A: real HDR cubemap — combine before exposure + tonemap.
		ambientTerm  = IBLAmbient(N, V, albedo, metallic, roughness);
		ambientTerm *= ao;  // Apply ambient occlusion
		directLight += ambientTerm;
		directLight *= ubo.Exposure;
		directLight  = ACESToneMapping(directLight);
	}
	else if (ubo.HasSky != 0u && ubo.UseColorIBL != 0u)
	{
		// Path B: display-space sky colour — tonemap first, add ambient after.
		directLight *= ubo.Exposure;
		directLight  = ACESToneMapping(directLight);
		ambientTerm  = IBLAmbient(N, V, albedo, metallic, roughness);
		ambientTerm *= ao;  // Apply ambient occlusion
		directLight += ambientTerm;
	}
	else
	{
		// Path C: no IBL — tonemap direct light, then add fallback ambient.
		directLight *= ubo.Exposure;
		directLight  = ACESToneMapping(directLight);
		ambientTerm  = ubo.FallbackAmbientColor * albedo;
		ambientTerm *= ao;  // Apply ambient occlusion
		directLight += ambientTerm;
	}

	vec3 finalColor = clamp(directLight, 0.0, 1.0);

	// sRGB gamma correction
	finalColor = pow(finalColor, vec3(1.0 / 2.2));

	// ===== MODIFIED: Include finalAlpha in output =====
	outColor = vec4(finalColor, finalAlpha);
}
