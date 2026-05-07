// Adaptive Sampling Utilities
// Provides functions for variance-based convergence detection

// Calculate luminance (perceptual brightness)
float Luminance(const vec3 color)
{
	return dot(color, vec3(0.299, 0.587, 0.114));
}

// Calculate relative variance between consecutive samples
// Lower variance = pixel is converging
float CalculateVariance(const vec3 sampleColor, const vec3 accumulatedMean, const uint sampleCount)
{
	if (sampleCount < 2)
		return 1.0;  // High variance initially

	// Simple variance estimation: compare new sample to accumulated mean
	vec3 diff = sampleColor - accumulatedMean;
	float variance = dot(diff, diff) / (3.0 * float(sampleCount));

	return variance;
}

// Check if pixel has converged based on variance threshold
bool HasConverged(const vec3 currentMean, const vec3 previousMean, const float threshold, const uint minSamples)
{
	// Don't converge until minimum samples reached
	if (minSamples > 0)
		return false;

	// Calculate relative difference between current and previous mean
	vec3 diff = currentMean - previousMean;
	float relativeVariance = dot(diff, diff);

	return relativeVariance < (threshold * threshold);
}

// Adaptive sample count based on scene complexity (estimated from variance)
uint AdaptiveSampleCount(const vec3 mean, const vec3 previousMean, const uint baseSamples, const uint minSamples)
{
	// If below minimum samples, always sample more
	if (baseSamples < minSamples)
		return minSamples;

	// Calculate color difference (complexity indicator)
	vec3 colorDiff = mean - previousMean;
	float complexity = length(colorDiff);

	// Higher complexity = more samples needed
	// Clamp between base samples and 2x for stability
	uint adaptiveSamples = uint(mix(
		float(baseSamples) * 0.5,   // Converged: reduce samples
		float(baseSamples) * 2.0,   // Complex: increase samples
		min(complexity * 2.0, 1.0)
	));

	return max(adaptiveSamples, minSamples);
}
