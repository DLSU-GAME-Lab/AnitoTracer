#version 460

// ── Uniform buffer: camera matrices ─────────────────────────────────────────
// Layout MUST match
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
} ubo;

// ── Per-vertex attributes (matches Assets::Vertex) ─────────────────────────
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in int  inMaterialIndex;

// ── Push constant: per-object model matrix ──────────────────────────────────
// Matches Assets::PushConstantModel
layout(push_constant) uniform PushConstant
{
	mat4 WorldMatrix;
} pc;

// ── Outputs to fragment shader ───────────────────────────────────────────────
layout(location = 0) out vec3  outWorldPos;
layout(location = 1) out vec3  outNormal;
layout(location = 2) out vec2  outTexCoord;
layout(location = 3) out flat int outMaterialIndex;

void main()
{
	vec4 worldPos    = pc.WorldMatrix * vec4(inPosition, 1.0);
	outWorldPos      = worldPos.xyz;
	// Normal matrix: world inverse-transpose then into view space
	outNormal        = normalize(mat3(transpose(inverse(pc.WorldMatrix))) * inNormal);
	outTexCoord      = inTexCoord;
	outMaterialIndex = inMaterialIndex;

	// Match legacy Graphics.vert: Projection * ModelView * WorldMatrix * pos
	gl_Position = ubo.Projection * ubo.ModelView * worldPos;
}
