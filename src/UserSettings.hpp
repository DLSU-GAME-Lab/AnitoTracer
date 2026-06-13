#pragma once

#include <glm/glm.hpp>

struct UserSettings final
{
	// Renderer Mode Enumeration
	enum class RendererMode
	{
		Legacy,        // Hardware ray tracing (VK_KHR_ray_tracing_pipeline)
		ComputeShader, // Compute shader ray tracer (software BVH)
		Game           // Real-time rasterization renderer
	};

	// Application
	bool Benchmark;

	// Benchmark
	bool BenchmarkNextScenes{};
	uint32_t BenchmarkMaxTime{};

	// Scene
	int SceneIndex;

	// Renderer
	RendererMode CurrentRendererMode = RendererMode::Legacy;
	bool IsRayTraced;
	bool AccumulateRays;
	bool MultiSampling;
	uint32_t aaValue;
	uint32_t NumberOfSamples;
	uint32_t NumberOfBounces;
	uint32_t MaxNumberOfSamples;

	// Compute shader: samples computed per-invocation inside a single dispatch.
	// Higher values converge faster per-frame but cost more GPU time per dispatch.
	// NOTE: Keep low (4-8) when using software BVH — no hardware acceleration structure.
	uint32_t SamplesPerInvocation = 4;

	// Camera
	bool rightClickToMoveCamera = true;
	float FieldOfView;
	float Aperture;
	float FocusDistance;

	// Physics
	float PhysicsTimestep = 1.f/60.f; //60fps

	// Profiler
	bool ShowHeatmap;
	float HeatmapScale;

	// Adaptive Sampling
	bool EnableAdaptiveSampling = true;
	float VarianceThreshold = 0.1f;
	uint32_t MinSamples = 8;

	// UI
	bool ShowSettings;
	bool ShowOverlay;

	inline static constexpr float FieldOfViewMinValue = 10.0f;
	inline static constexpr float FieldOfViewMaxValue = 90.0f;

	// Ray Visualization
	uint32_t MaxRays = 16;

	// Game Renderer settings (only used when CurrentRendererMode == Game)
	struct GameSettings
	{
		//All are off by default desu~

		// Tone mapping
		float Exposure = 0.00001f;  // Scene exposure for direct lighting (physical units: 1/100000 tuned for 500k-intensity lights)

		// Bloom
		bool  EnableBloom     = false;
		float BloomThreshold  = 1.0f;
		float BloomIntensity  = 0.5f;

		// SSAO
		bool  EnableSSAO  = false;
		float SSAORadius  = 0.5f;
		float SSAOBias    = 0.025f;

		// Temporal Anti-Aliasing
		bool EnableTAA = false;

		// Screen-Space Reflections
		bool EnableSSR = false;

		// Image-Based Lighting
		bool EnableIBL = false;

		// When true, IBL sky irradiance is tinted by IBLSkyColor instead of
		// sampling the raw HDR cubemap. Useful for stylised / non-photorealistic looks.
		bool      UseColorIBL = true;
		glm::vec3 IBLSkyColor = glm::vec3(0.529f, 0.808f, 0.922f); // Light blue default

		// Fallback Ambient Color (used when IBL is disabled)
		glm::vec3 FallbackAmbientColor = glm::vec3(0.03f); // Dim ambient — prevents pure-black dark side without IBL

	} Game;

	bool RequiresAccumulationReset(const UserSettings& prev) const
	{
		return
			IsRayTraced != prev.IsRayTraced ||
			AccumulateRays != prev.AccumulateRays ||
			NumberOfBounces != prev.NumberOfBounces ||
			FieldOfView != prev.FieldOfView ||
			Aperture != prev.Aperture ||
			FocusDistance != prev.FocusDistance ||
			EnableAdaptiveSampling != prev.EnableAdaptiveSampling ||
				VarianceThreshold != prev.VarianceThreshold ||
				MinSamples != prev.MinSamples ||
				SamplesPerInvocation != prev.SamplesPerInvocation;
	}
};
