#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#include "Material.glsl"

layout(binding = 4) readonly buffer VertexArray { float Vertices[]; };
layout(binding = 5) readonly buffer IndexArray { uint Indices[]; };
layout(binding = 6) readonly buffer MaterialArray { Material[] Materials; };
layout(binding = 7) readonly buffer LightsArray { LightProperties[] Lights; }; 
layout(binding = 8) readonly buffer OffsetArray { uvec2[] Offsets; };
layout(binding = 9) uniform sampler2D[] TextureSamplers;
layout(binding = 10) readonly buffer SphereArray { vec4[] Spheres; };

#include "Scatter.glsl"
#include "Vertex.glsl"

hitAttributeEXT vec4 Sphere;
rayPayloadInEXT RayPayload Ray;

// Point Light Constants
const int lightType = 0;
const vec3 lightPos = vec3(2600, 20.0, 0);
const vec4 ambientColor = vec4(1.0, 1.0, 1.0, 0.02);
const vec4 lightColor = vec4(0.0, 0.4, 0.5, 1000000.0f);

vec2 GetSphereTexCoord(const vec3 point)
{
	const float phi = atan(point.x, point.z);
	const float theta = asin(point.y);
	const float pi = 3.1415926535897932384626433832795;

	return vec2
	(
		(phi + pi) / (2* pi),
		1 - (theta + pi /2) / pi
	);
}

vec3 calculatePointLight(LightProperties pl, vec3 worldPos, vec3 normal) 
{
	// Computing the coordinates of the hit position
	const vec3 worldNrm = normalize(transpose(inverse(mat3(gl_ObjectToWorldEXT))) * normal);

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

vec3 calculateDirectionalLight(LightProperties dl, vec3 worldPos, vec3 normal) 
{
	// Computing the coordinates of the hit position
	const vec3 worldNrm = normalize(transpose(inverse(mat3(gl_ObjectToWorldEXT))) * normal);

	// Compute the diffuse light.
	vec3 lightDir = dl.LightPos.xyz - worldPos;
	vec3 lighting = dl.LightColor.rgb * max(dot(worldNrm, normalize(lightDir)), 0);

	return lighting;
}

void main()
{
	// Get the material.
	const uvec2 offsets = Offsets[gl_InstanceCustomIndexEXT];
	const uint indexOffset = offsets.x;
	const uint vertexOffset = offsets.y;
	const Vertex v0 = UnpackVertex(vertexOffset + Indices[indexOffset]);
	const Material material = Materials[v0.MaterialIndex];

	// Compute the ray hit point properties.
	const vec4 sphere = Spheres[gl_InstanceCustomIndexEXT];
	const vec3 center = sphere.xyz;
	const float radius = sphere.w;
	const vec3 point = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
	const vec3 normal = (point - center) / radius;
	const vec2 texCoord = GetSphereTexCoord(normal);

	// For lighting computations.
	const vec3 pos = center;
	const vec3 worldPos = vec3(gl_ObjectToWorldEXT * vec4(pos, 1.0));  // Transforming the position to world space
	
	LightProperties pl = InitializeTestPLProperties(); // Adding point light.
	LightProperties dl = InitializeTestDLProperties(); // Adding point light.
	vec3 lighting = vec3(0);
	lighting += calculatePointLight(pl, worldPos, normal);
	lighting += calculateDirectionalLight(dl, worldPos, normal);
	

	Ray = Scatter(material, gl_WorldRayDirectionEXT, normal, texCoord, gl_HitTEXT, Ray.RandomSeed, lighting, 0);
}
