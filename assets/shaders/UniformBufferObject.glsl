
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
	uint NumberOfBounces;
	uint RandomSeed;
	uint MaxRays;
	bool HasSky;
	bool ShowHeatmap;
};

struct PushConstantModel {
	mat4 WorldMatrix;
};