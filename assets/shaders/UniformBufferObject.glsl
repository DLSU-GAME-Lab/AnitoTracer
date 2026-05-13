struct UniformBufferObject
{
	mat4 ModelView;
	mat4 Projection;
	mat4 ModelViewInverse;
	mat4 ProjectionInverse;
	float Aperture;
	float FocusDistance;
	float HeatmapScale;
	uint TotalNumberOfSamples;
	uint NumberOfSamples;
	uint SamplesPerInvocation;    // NEW: Samples computed inside a single shader dispatch (decoupled from cross-frame accumulation)
	uint NumberOfBounces;
	uint RandomSeed;
	uint MaxRays;
	bool HasSky;
	bool ShowHeatmap;
	bool EnableAdaptiveSampling;  // NEW: Enable adaptive sampling
	float VarianceThreshold;       // NEW: Convergence threshold (0.001 typical)
	uint MinSamples;               // NEW: Minimum samples per pixel before adaptive termination
	vec3 FallbackAmbientColor;     // RGB fallback ambient when IBL is disabled
};

struct PushConstantModel {
	mat4 WorldMatrix;
};