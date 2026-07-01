#version 460
// ── Shadow Map Vertex Shader ──────────────────────────────────────────────────
// Depth-only pass: transforms each vertex into the selected directional light's
// clip space.  One sub-pass is recorded per active shadow-casting light; the
// correct LightViewProj row is selected via pc.LightIndex.

#define MAX_SHADOW_LIGHTS 4

// Binding 0: array of light view-projection matrices (one per shadow slot).
layout(binding = 0) uniform ShadowUBO
{
	mat4 LightViewProj[MAX_SHADOW_LIGHTS];
	uint Count;
} shadowUBO;

// Push constant: per-object model matrix + index of the light being rendered.
// Total size = 64 (mat4) + 4 (uint) = 68 bytes — within the 128-byte minimum.
layout(push_constant) uniform PushConstant
{
	mat4 WorldMatrix;
	uint LightIndex;
} pc;

// Only the position attribute is consumed; the rest of the Vertex data is
// bound via the scene vertex buffer but ignored by this shader.
layout(location = 0) in vec3 inPosition;

void main()
{
	gl_Position = shadowUBO.LightViewProj[pc.LightIndex] * pc.WorldMatrix * vec4(inPosition, 1.0);
}
