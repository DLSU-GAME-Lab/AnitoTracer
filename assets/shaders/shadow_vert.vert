#version 460
// ── Shadow Map Vertex Shader ──────────────────────────────────────────────────
// Depth-only pass: transforms each vertex into the directional light's clip space.
// No colour output — the driver only writes gl_Position.z to the depth attachment.

// Binding 0: light view-projection matrix (ShadowUBO)
layout(binding = 0) uniform ShadowUBO
{
	mat4 LightViewProj;
} shadowUBO;

// Push constant: per-object model-to-world matrix (matches main pass layout)
layout(push_constant) uniform PushConstant
{
	mat4 WorldMatrix;
} pc;

// Only the position attribute is consumed; the rest of the Vertex data is
// bound via the scene vertex buffer but ignored by this shader.
layout(location = 0) in vec3 inPosition;

void main()
{
	gl_Position = shadowUBO.LightViewProj * pc.WorldMatrix * vec4(inPosition, 1.0);
}
