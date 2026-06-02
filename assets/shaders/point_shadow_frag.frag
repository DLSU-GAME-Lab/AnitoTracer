#version 460

// Input from vertex shader
layout(location = 0) in vec3 inWorldPos;

// UBO for light positions
layout(binding = 0) uniform PointShadowUBO
{
	mat4  CubemapViewProj[4 * 6];
	vec4  LightPositions[4];
	uint  Count;
	float FarPlane;
	float _pad[2];
} pointShadowUBO;

// Push constants (vertex + fragment share the same range)
layout(push_constant) uniform PushConstant {
	mat4  WorldMatrix;
	uint  LightIndex;
	uint  CubemapFaceIdx;
} pc;

void main()
{
	// Write LINEAR depth (distance / farPlane) so that the main pass can
	// compare against the same metric without a projection-space mismatch.
	// Hardware depth (gl_FragCoord.z) is non-linear and would not match
	// the linear reference produced by the main shader.
	vec3  lightPos  = pointShadowUBO.LightPositions[pc.LightIndex].xyz;
	float linearDep = length(inWorldPos - lightPos) / pointShadowUBO.FarPlane;
	gl_FragDepth    = linearDep;
}
