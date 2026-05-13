#version 460
#extension GL_EXT_nonuniform_qualifier : require

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

// ── Inputs from vertex shader ─────────────────────────────────────────────────
layout(location = 0) in vec3  inWorldPos;
layout(location = 1) in vec3  inNormal;
layout(location = 2) in vec2  inTexCoord;
layout(location = 3) in flat int inMaterialIndex;

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
	vec3  H     = normalize(V + L);
	float NdotV = max(dot(N, V), 0.0001);
	float NdotL = max(dot(N, L), 0.0001);
	float NdotH = max(dot(N, H), 0.0);
	float HdotV = max(dot(H, V), 0.0);

	vec3  F0  = mix(vec3(0.04), albedo, metallic);
	float D   = D_GGX(NdotH, roughness);
	float G   = G_SmithSchlick(NdotV, NdotL, roughness);
	vec3  F   = F_Schlick(HdotV, F0);

	vec3 kD       = (1.0 - F) * (1.0 - metallic);
	vec3 diffuse  = kD * albedo / PI;
	vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);

	return (diffuse + specular) * NdotL;
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
// main
// ─────────────────────────────────────────────────────────────────────────────
void main()
{
	Material mat = materials[inMaterialIndex];

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

	// Accumulate contributions from all lights in the buffer
	vec3 finalColor = vec3(0.0);
	uint lightCount = uint(lights.length());

	// Base ambient contribution from the configured FallbackAmbientColor (always applied)
	finalColor += ubo.FallbackAmbientColor * albedo;

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

		float intensity   = light.LightColor.a;
		vec3  lightColor  = light.LightColor.rgb * intensity;

		finalColor += CookTorranceBRDF(N, V, L, albedo, metallic, roughness)
					  * lightColor * attenuation;
	}

	// Tone mapping (Reinhard) + gamma correction
	finalColor = finalColor / (finalColor + vec3(1.0));
	finalColor = pow(finalColor, vec3(1.0 / 2.2));

	outColor = vec4(finalColor, 1.0);
}
