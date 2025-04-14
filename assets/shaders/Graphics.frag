#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#include "Material.glsl"

layout(binding = 1) readonly buffer MaterialArray { Material[] Materials; };
layout(binding = 2) uniform sampler2D[] TextureSamplers;
layout(binding = 3) readonly buffer LightsArray { LightProperties[] Lights; }; 

layout(location = 0) in vec3 FragColor;
layout(location = 1) in vec3 FragNormal;
layout(location = 2) in vec2 FragTexCoord;
layout(location = 3) in flat int FragMaterialIndex;
layout(location = 4) in vec3 worldPos;

layout(location = 0) out vec4 OutColor;

const vec4 dirLightColor = vec4(1.0);
const vec3 dirLightDir = normalize(vec3(5.0, 4.0, 3.0));

vec3 calculatePointLight(LightProperties pl, vec3 worldPos, vec3 normal) 
{
	// Compute the diffuse light.
	vec3 lightDir = pl.LightPos.xyz - worldPos;
	float attenuation = 1.0 / dot(lightDir, lightDir);

	// Compute the light colors and intensity.
	vec3 lightCol = pl.LightColor.xyz * pl.LightColor.w * attenuation;
	vec3 ambientLight = pl.AmbientColor.xyz * pl.AmbientColor.w;
	vec3 diffuseLight = lightCol * max(dot(normal, normalize(lightDir)), 0);
	
	vec3 lighting = diffuseLight + ambientLight;
	 
	return lighting;
}
 
vec3 calculateDirectionalLight(LightProperties dl, vec3 worldPos, vec3 normal) 
{
	vec3 lightDir = -normalize(dl.LightPos);

	vec3 ambientColor = dl.AmbientColor.rgb * dl.AmbientColor.w;

    float diffuseFactor = max(dot(normal, lightDir), 0.f);
    vec3 diffuseColor = dl.LightColor.rgb * dl.LightColor.w * diffuseFactor;
	 
	vec3 lighting = (ambientColor + diffuseColor) * 0.2;
	return lighting;
} 

vec3 calculateSpotLight(LightProperties sl, vec3 worldPos, vec3 normal) 
{         
	float cutoff = cos(radians(90.0)); // Convert degrees to radians and compute cosine
	vec3 lightDir = normalize(sl.LightPos.xyz - worldPos); // Direction from light to hit point
	vec3 lightDirection = vec3(0, -1, 0);
	vec3 spotDir = normalize(lightDirection); // Direction of the spot light 
	float spotFactor = dot(lightDir, -spotDir); // Cosine of the angle between lightDir and spotDir
	 
	if (spotFactor > cutoff) {
		vec3 lighting = calculatePointLight(sl, worldPos, normal);
		return lighting * (1.0 - (1.0 - spotFactor) * 1.0 / (1.0 - cutoff));
	}
	return vec3(0.0); // Return no light if outside the spot light cone
}


void main() 
{
	const int textureId = Materials[FragMaterialIndex].DiffuseTextureId;
	const int normalTextureId = Materials[FragMaterialIndex].NormalTextureId;
	const float normalStrength = Materials[FragMaterialIndex].Normal;
	//const float d = max(dot(dirLightDir, normalize(FragNormal)), 0.2);
	
	// Base normal
	vec3 normal = normalize(FragNormal);

	// Apply normal map if available
	if (normalTextureId >= 0) {
		vec3 sampledNormal = texture(TextureSamplers[normalTextureId], FragTexCoord).rgb;
		sampledNormal = normalize(sampledNormal * 2.0 - 1.0);
		normal = normalize(mix(normal, FragNormal, 1.0 - normalStrength) * normal); // scale blend
	}

	// Compute base directional light shading
	float d = max(dot(dirLightDir, normal), 0.2);  // Soft minimum lighting

	vec3 c = FragColor * d;
	if (textureId >= 0)
	{
		c *= texture(TextureSamplers[textureId], FragTexCoord).rgb;
	}
	
	vec3 lighting = vec3(0.0);
	
	for (int i = 0; i < Lights.length(); i++) {
		// Point Light
		if (Lights[i].LightType == PointLight) { 
			lighting += calculatePointLight(Lights[i], worldPos, normal);
		} 
		// Directional Light
		else if (Lights[i].LightType == DirectionalLight) { 
			lighting += calculateDirectionalLight(Lights[i], worldPos, normal);
		} 
		// Spot Light
		else if (Lights[i].LightType == SpotLight) { 
			//lighting += calculateSpotLight(Lights[i], worldPos, normal);
		}
	}

	// Test point light
	LightProperties pl = InitializeTestPLProperties(); // Adding point light.
	lighting += calculatePointLight(pl, worldPos, normal);

	OutColor = vec4(lighting,1) + vec4(c, 1);
	
    //OutColor = dirLightColor * vec4(c, 1);
}