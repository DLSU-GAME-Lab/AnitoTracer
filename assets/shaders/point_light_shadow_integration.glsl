#version 460
#extension GL_EXT_nonuniform_qualifier : require

// ══════════════════════════════════════════════════════════════════════════════
// POINT LIGHT SHADOW BINDINGS (NEW)
// ══════════════════════════════════════════════════════════════════════════════

#define MAX_POINT_SHADOW_LIGHTS 4

// Binding 7: Cubemap shadow maps with PCF hardware compare
layout(binding = 7) uniform samplerCubeShadow pointShadowMaps[MAX_POINT_SHADOW_LIGHTS];

// Binding 8: Point light cubemap VP matrices and positions
layout(binding = 8) uniform PointShadowUBO
{
	mat4  CubemapViewProj[MAX_POINT_SHADOW_LIGHTS * 6];  // 6 VP matrices per light
	vec4  LightPositions[MAX_POINT_SHADOW_LIGHTS];       // Light positions (world-space)
	uint  Count;                                          // Number of active point lights
	float _pad[3];
} pointShadowUBO;

// ══════════════════════════════════════════════════════════════════════════════
// POINT LIGHT SHADOW HELPER FUNCTIONS
// ══════════════════════════════════════════════════════════════════════════════

/// @brief Sample point light shadow at a world position.
/// @param lightIdx  Index of the point light (0 to Count-1)
/// @param worldPos  Fragment world position
/// @param maxDist   Maximum shadow distance (typically the light's far plane, e.g., 1000.0)
/// @return Shadow factor: 1.0 = lit, 0.0 = in shadow, (0,1) = penumbra (PCF)
float SamplePointLightShadow(uint lightIdx, vec3 worldPos, float maxDist)
{
	if (lightIdx >= pointShadowUBO.Count)
		return 1.0;  // Fully lit if index out of range

	vec3 lightPos = pointShadowUBO.LightPositions[lightIdx].xyz;
	vec3 toLight = worldPos - lightPos;
	float distance = length(toLight);

	// Normalize the direction for cubemap lookup
	vec3 cubemapDir = normalize(toLight);

	// Sample the cubemap shadow with hardware PCF compare
	// samplerCubeShadow compares the depth automatically
	// Reference depth = distance / maxDist (normalized to [0, 1])
	float shadow = texture(pointShadowMaps[lightIdx], 
						   vec4(cubemapDir, distance / maxDist));

	return shadow;
}

/// @brief Evaluate point light contribution with shadow.
/// @param light       The light properties from the light buffer
/// @param lightIdx    Index of the point light in the shadow UBO
/// @param fragPos     Fragment world position
/// @param normal      Fragment normal
/// @param viewDir     Normalized view direction (from fragment towards camera)
/// @param albedo      Surface albedo
/// @param metallic    Metallic parameter
/// @param roughness   Roughness parameter
/// @return Lit color contribution (albedo * light color * attenuation * shadow)
vec3 EvaluatePointLightWithShadow(
	LightProperties light,
	uint lightIdx,
	vec3 fragPos,
	vec3 normal,
	vec3 viewDir,
	vec3 albedo,
	float metallic,
	float roughness)
{
	vec3 lightPos = light.LightPos.xyz;
	vec3 toLight = lightPos - fragPos;
	float dist = length(toLight);
	vec3 lightDir = normalize(toLight);

	// Inverse-square law attenuation
	float attenuation = 1.0 / (dist * dist + 0.01);  // small epsilon prevents division by zero

	// Get light color and intensity
	float intensity = light.LightColor.a;
	vec3 lightColor = light.LightColor.rgb * intensity;

	// Compute PBR contribution (reuse existing Cook-Torrance function)
	vec3 brdfContribution = CookTorranceBRDF(normal, viewDir, lightDir, 
											  albedo, metallic, roughness);

	// Sample point light shadow
	float shadow = SamplePointLightShadow(lightIdx, fragPos, 1000.0);  // 1000.0 = far plane

	// Combine: BRDF * light color * attenuation * shadow
	return brdfContribution * lightColor * attenuation * shadow;
}

// ══════════════════════════════════════════════════════════════════════════════
// INTEGRATION POINT: Call in main()
// ══════════════════════════════════════════════════════════════════════════════

/*
In the main() function, after evaluating directional lights, add:

	// Point light shadows
	uint pointLightIdx = 0;
	for (const auto& light : lights)
	{
		if (light.LightType == 0u)  // PointLight enum value
		{
			vec3 pointContrib = EvaluatePointLightWithShadow(
				light,
				pointLightIdx,
				inWorldPos,
				normal,
				viewDir,
				albedo,
				metallic,
				roughness);
			finalColor += pointContrib;
			pointLightIdx++;
		}
	}
*/
