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

	// Normal matrix: world inverse-transpose then into view space
	// Note: This produces world-space normals (not view-space despite the name)
	outNormal        = normalize(mat3(transpose(inverse(pc.WorldMatrix))) * inNormal);
	outTexCoord      = inTexCoord;
	outMaterialIndex = inMaterialIndex;

	// Transform normal to world space for tangent computation
	vec3 worldNormal = mat3(pc.WorldMatrix) * inNormal;

	// Compute an approximate tangent perpendicular to the normal.
	// This works without requiring tangent data in the mesh.
	// The method: find an arbitrary vector not parallel to the normal,
	// then orthogonalize it to create a consistent tangent basis.
	//
	// Strategy: Check which axis (Z or X) is least aligned with the normal
	vec3 approximateTangent;
	if (abs(worldNormal.z) < 0.9)
	{
		// If normal is not primarily along Z, use Z-axis as reference
		approximateTangent = vec3(0.0, 0.0, 1.0);
	}
	else
	{
		// If normal is primarily along Z, use X-axis as reference
		approximateTangent = vec3(1.0, 0.0, 0.0);
	}

	// Orthogonalize: remove the component of approximateTangent that's parallel to normal
	approximateTangent = normalize(approximateTangent - dot(approximateTangent, worldNormal) * worldNormal);

	// Create final tangent via cross product for consistency
	// The cross product ensures we get a vector orthogonal to the normal
	outTangent = normalize(cross(worldNormal, approximateTangent));

	// Match legacy Graphics.vert: Projection * ModelView * WorldMatrix * pos
	gl_Position = ubo.Projection * ubo.ModelView * worldPos;
}
