#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_control_flow_attributes : require

#include "Material.glsl"
#include "RayPayload.glsl"
#include "Random.glsl"

layout(binding = 4) readonly buffer VertexArray { float Vertices[]; };
layout(binding = 5) readonly buffer IndexArray { uint Indices[]; };
layout(binding = 6) readonly buffer MaterialArray { Material[] Materials; };
layout(binding = 7) readonly buffer LightsArray { LightProperties[] Lights; }; 
layout(binding = 8) readonly buffer OffsetArray { uvec2[] Offsets; };
layout(binding = 9) uniform sampler2D[] TextureSamplers;

#include "Vertex.glsl"
#include "Scatter.glsl"

hitAttributeEXT vec2 HitAttributes;
rayPayloadInEXT RayPayload Ray;

vec2 Mix(vec2 a, vec2 b, vec2 c, vec3 barycentrics)
{
	return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec3 Mix(vec3 a, vec3 b, vec3 c, vec3 barycentrics) 
{
    return a * barycentrics.x + b * barycentrics.y + c * barycentrics.z;
}

vec3 calculatePointLight(LightProperties pl, vec3 worldPos, const vec3 worldNrm) 
{
	// Compute the diffuse light.
	vec3 lightDir = pl.LightPos.xyz - worldPos;
	float attenuation = 1.0 / dot(lightDir, lightDir);

	// Compute the light colors and intensity.
	vec3 lightCol = pl.LightColor.xyz * pl.LightColor.w * attenuation;
	vec3 ambientLight = pl.AmbientColor.xyz * pl.AmbientColor.w;
	vec3 diffuseLight = lightCol * max(dot(worldNrm, normalize(lightDir)), 0);

	vec3 lighting = diffuseLight + ambientLight;

	return lighting; 
}
 
vec3 calculateDirectionalLight(LightProperties dl, const vec3 worldNrm) 
{
	vec3 ambientColor = dl.AmbientColor.rgb * dl.AmbientColor.w;

	float diffuseFactor = max(dot(worldNrm, dl.LightDir), 0.f);
	vec3 diffuseColor = dl.LightColor.rgb * dl.LightColor.w * diffuseFactor;

	vec3 lighting = (ambientColor + diffuseColor) * 0.0001;
	return lighting;
}

vec3 calculateSpotLight2(LightProperties sl, vec3 worldPos, vec3 normal) 
{         
	float cutoff = cos(radians(90.0)); // Convert degrees to radians and compute cosine
	// vec3 lightDir = normalize(sl.LightPos.xyz - worldPos); // Direction from light to hit point
	// vec3 lightDirection = vec3(0, -1, 0);
	vec3 spotDir = normalize(sl.LightDir); // Direction of the spot light 
	float spotFactor = dot(sl.LightDir, -spotDir); // Cosine of the angle between lightDir and spotDir
	 
	if (spotFactor > cutoff) {
		vec3 lighting = calculatePointLight(sl, worldPos, normal);
		return lighting * (1.0 - (1.0 - spotFactor) * 1.0 / (1.0 - cutoff));
	}
	return vec3(0.0); // Return no light if outside the spot light cone
}

vec3 calculateSpotLight(LightProperties sl, vec3 worldPos, const vec3 worldNrm)  
{
    // Vector from light to fragment
    vec3 fragToLight = worldPos - sl.LightPos;
    float distance = length(fragToLight);
    vec3 lightDir = normalize(-fragToLight); // Direction from light to fragment

    // Normalize spotlight direction (already in world space)
    vec3 spotDir = normalize(sl.LightDir);

    // Compute the angle between light direction and spotlight direction
    float theta = dot(lightDir, spotDir); // Cosine of angle between them

    // Define cutoff angles (in cosine space)
    float innerCutoff = cos(radians(20.0));
    float outerCutoff = cos(radians(30.0));

    // Compute smooth falloff for spotlight edge
    float epsilon = innerCutoff - outerCutoff;
    float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);

    // Compute attenuation
    float attenuation = 1.0 / (distance * distance);

    // Diffuse component
    float diff = max(dot(worldNrm, lightDir), 0.0);
    vec3 diffuse = sl.LightColor.rgb * sl.LightColor.w * diff * attenuation * intensity;

    // Ambient component
    vec3 ambient = sl.AmbientColor.rgb * sl.AmbientColor.w;

    return ambient + diffuse;
}
void main()
{
    // Get the material.
    const uvec2 offsets = Offsets[gl_InstanceCustomIndexEXT];
    const uint indexOffset = offsets.x;
    const uint vertexOffset = offsets.y;
    const Vertex v0 = UnpackVertex(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 0]);
    const Vertex v1 = UnpackVertex(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 1]);
    const Vertex v2 = UnpackVertex(vertexOffset + Indices[indexOffset + gl_PrimitiveID * 3 + 2]);
    const Material material = Materials[v0.MaterialIndex];

    // Compute the ray hit point properties.    
    const vec3 barycentrics = vec3(1.0 - HitAttributes.x - HitAttributes.y, HitAttributes.x, HitAttributes.y);
    const vec3 normal = normalize(Mix(v0.Normal, v1.Normal, v2.Normal, barycentrics));
    const vec2 texCoord = Mix(v0.TexCoord, v1.TexCoord, v2.TexCoord, barycentrics);

    // For lighting computations.
    const vec3 pos = Mix(v0.Position, v1.Position, v2.Position, barycentrics);
    const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));  // Transforming the position to world space

    // PRE-COMPUTE WORLD NORMAL ONCE - Optimization: avoid redundant matrix operations in light loop
    const vec3 worldNrm = normalize(transpose(inverse(mat3(gl_ObjectToWorldEXT))) * normal);

    vec3 lighting = vec3(0);

    if (Lights.length() == 0) {
        // fallback lighting if needed
    } else {
        for (int i = 0; i < Lights.length(); i++) {
            if (Lights[i].LightType == PointLight) {
                lighting += calculatePointLight(Lights[i], worldPos, worldNrm);
            } else if (Lights[i].LightType == DirectionalLight) {
                lighting += calculateDirectionalLight(Lights[i], worldNrm);
            } else if (Lights[i].LightType == SpotLight) {
                lighting += calculateSpotLight(Lights[i], worldPos, worldNrm);
            }
        }
    }

    vec4 texColor = vec4(1.0);
    if (material.DiffuseTextureId >= 0)
        texColor = texture(TextureSamplers[material.DiffuseTextureId], texCoord);

    Ray = Scatter(material, gl_WorldRayDirectionEXT, normal, texCoord, gl_HitTEXT, Ray.RandomSeed, lighting, Ray.anyHitTriggered);
}