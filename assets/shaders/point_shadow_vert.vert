#version 460

// Push constants: world matrix + light index + cubemap face index
layout(push_constant) uniform PushConstant {
	mat4  WorldMatrix;
	uint  LightIndex;
	uint  CubemapFaceIdx;
} pc;

// Per-vertex input from scene geometry
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

// UBO: cubemap VP matrices (6 per light) and light positions
layout(binding = 0) uniform PointShadowUBO
{
	mat4  CubemapViewProj[4 * 6];  // 4 lights * 6 faces per light
	vec4  LightPositions[4];        // Light positions (world-space)
	uint  Count;
	float FarPlane;
	float _pad[2];
} pointShadowUBO;

// Output to fragment shader
layout(location = 0) out vec3 outWorldPos;

void main()
{
	// Transform vertex to world space
	outWorldPos = (pc.WorldMatrix * vec4(inPosition, 1.0)).xyz;

	// Get the VP matrix for this light + cubemap face
	uint vpIndex = pc.LightIndex * 6u + pc.CubemapFaceIdx;
	mat4 vp = pointShadowUBO.CubemapViewProj[vpIndex];

	// Transform world position through the cubemap VP matrix for this face
	vec4 clipPos = vp * vec4(outWorldPos, 1.0);

	gl_Position = clipPos;
}

