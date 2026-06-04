// Normal mapping helper functions and structures for game_frag.frag
// These should be added to the fragment shader

// ---------- GameRendererMaterialProperties buffer (binding 13) ----------------
// Layout must match Vulkan::Game::GameRendererMaterialProperties
struct GameRenderMaterialProperties
{
	int   NormalMapTextureId;
	float NormalMapStrength;
	int   MetallicMapTextureId;
	float MetallicValue;
	int   RoughnessMapTextureId;
	float RoughnessValue;
	int   AOMapTextureId;
	float AOStrength;
};

layout(binding = 12) uniform sampler2D normalMaps[];        // Normal map textures
layout(binding = 13) readonly buffer GameRenderMatProps   { GameRenderMaterialProperties materialProperties[]; };
layout(binding = 14) uniform sampler2D metallicMaps[];      // Metallic texture maps
layout(binding = 15) uniform sampler2D roughnessMaps[];     // Roughness texture maps
layout(binding = 16) uniform sampler2D aoMaps[];            // Ambient occlusion maps

// Input from vertex shader (add to existing inputs)
// layout(location = 4) in vec3 inTangent;

// ---------- TBN Matrix Construction --------
// Constructs tangent-bitangent-normal (TBN) matrix for transforming normals from tangent space
mat3 ConstructTBNMatrix(vec3 normal, vec3 tangent)
{
	// Gram-Schmidt orthogonalization to ensure orthogonal basis
	tangent = normalize(tangent - dot(tangent, normal) * normal);
	vec3 bitangent = cross(normal, tangent);
	return mat3(tangent, bitangent, normal);
}

// ---------- Normal Map Sampling --------
// Samples and applies normal maps to the interpolated normal
// Returns the transformed normal in world space
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

	// Apply normal map strength
	sampledNormal = normalize(mix(vec3(0.0, 0.0, 1.0), sampledNormal, matProps.NormalMapStrength));

	// Construct TBN matrix and transform sampled normal to world space
	mat3 tbnMatrix = ConstructTBNMatrix(normal, tangent);
	vec3 worldNormal = normalize(tbnMatrix * sampledNormal);

	return worldNormal;
}

// ---------- Metallic Map Sampling --------
float SampleMetallicMap(int rematerialIndex, float defaultMetallic, vec2 texCoord)
{
	GameRenderMaterialProperties matProps = materialProperties[rematerialIndex];

	if (matProps.MetallicMapTextureId < 0)
		return defaultMetallic;

	// Sample metallic map (typically uses R channel or grayscale)
	float sampledMetallic = texture(metallicMaps[nonuniformEXT(matProps.MetallicMapTextureId)], texCoord).r;

	return mix(matProps.MetallicValue, sampledMetallic, 1.0);
}

// ---------- Roughness Map Sampling --------
float SampleRoughnessMap(int rematerialIndex, float defaultRoughness, vec2 texCoord)
{
	GameRenderMaterialProperties matProps = materialProperties[rematerialIndex];

	if (matProps.RoughnessMapTextureId < 0)
		return defaultRoughness;

	// Sample roughness map (typically uses R channel or grayscale)
	float sampledRoughness = texture(roughnessMaps[nonuniformEXT(matProps.RoughnessMapTextureId)], texCoord).r;

	// Linear interpolation between material's default and sampled value
	return mix(matProps.RoughnessValue, sampledRoughness, 1.0);
}

// ---------- Ambient Occlusion Map Sampling --------
float SampleAOMap(int rematerialIndex, vec2 texCoord)
{
	GameRenderMaterialProperties matProps = materialProperties[rematerialIndex];

	if (matProps.AOMapTextureId < 0)
		return 1.0; // No occlusion

	// Sample AO map (typically uses R channel or grayscale)
	float sampledAO = texture(aoMaps[nonuniformEXT(matProps.AOMapTextureId)], texCoord).r;

	// AO strength controls how much the sampled AO affects the result
	return mix(1.0, sampledAO, matProps.AOStrength);
}
