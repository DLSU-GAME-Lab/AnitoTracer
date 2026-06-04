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

// ── TBN matrix support for normal mapping ──────────────────────────────────
// Tangent vector for TBN matrix computation (used for normal map sampling in fragment shader)
layout(location = 4) out vec3  outTangent;
// Note: Bitangent is computed in the fragment shader as cross(normal, tangent) for efficiency

void main()
{
	vec4 worldPos    = pc.WorldMatrix * vec4(inPosition, 1.0);
	outWorldPos      = worldPos.xyz;

	// Normal matrix: world inverse-transpose (correct for handling non-uniform scaling)
	// This MUST be used for both normal and tangent to ensure consistent TBN matrix
	mat3 normalMatrix = mat3(transpose(inverse(pc.WorldMatrix)));
	outNormal        = normalize(normalMatrix * inNormal);
	outTexCoord      = inTexCoord;
	outMaterialIndex = inMaterialIndex;

	// ── Tangent computation using the SAME normal matrix for consistency ──
	// This ensures the TBN matrix constructed in the fragment shader is orthonormal
	vec3 worldNormal = normalize(normalMatrix * inNormal);

	// Compute a robust tangent perpendicular to the normal.
	// Uses a numerically stable approach: find the axis LEAST aligned with the normal,
	// then use cross product to generate a stable perpendicular vector.
	// This avoids Gram-Schmidt instability when the normal is nearly parallel to a reference axis.

	vec3 absNormal = abs(worldNormal);
	vec3 referenceAxis;

	// Select the axis least aligned with the normal (to maximize numerical stability)
	if (absNormal.x <= absNormal.y && absNormal.x <= absNormal.z)
	{
		// X-axis is least aligned
		referenceAxis = vec3(1.0, 0.0, 0.0);
	}
	else if (absNormal.y <= absNormal.z)
	{
		// Y-axis is least aligned
		referenceAxis = vec3(0.0, 1.0, 0.0);
	}
	else
	{
		// Z-axis is least aligned
		referenceAxis = vec3(0.0, 0.0, 1.0);
	}

	// Cross product is ALWAYS perpendicular and numerically stable
	// when the reference axis is least-aligned with the normal
	outTangent = normalize(cross(referenceAxis, worldNormal));

	// Match legacy Graphics.vert: Projection * ModelView * WorldMatrix * pos
	gl_Position = ubo.Projection * ubo.ModelView * worldPos;
}
