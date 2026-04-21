#pragma once

struct UserSettings final
{
	// Renderer Mode Enumeration
	enum class RendererMode
	{
		Legacy,
		ComputeShader
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

	// Camera
	float FieldOfView;
	float Aperture;
	float FocusDistance;

	// Profiler
	bool ShowHeatmap;
	float HeatmapScale;

	// UI
	bool ShowSettings;
	bool ShowOverlay;

	inline static constexpr float FieldOfViewMinValue = 10.0f;
	inline static constexpr float FieldOfViewMaxValue = 90.0f;

	// Ray Visualization
	uint32_t MaxRays = 16;

	bool RequiresAccumulationReset(const UserSettings& prev) const
	{
		return
			IsRayTraced != prev.IsRayTraced ||
			AccumulateRays != prev.AccumulateRays ||
			NumberOfBounces != prev.NumberOfBounces ||
			FieldOfView != prev.FieldOfView ||
			Aperture != prev.Aperture ||
			FocusDistance != prev.FocusDistance;
	}
};
