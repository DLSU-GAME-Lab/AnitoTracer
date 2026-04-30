// Generates a seed
// https://github.com/nvpro-samples/optix_prime_baking/blob/332a886f1ac46c0b3eea9e89a59593470c755a0e/random.h
// https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR/tree/master/ray_tracing_jitter_cam
// https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
uint InitRandomSeed(uint val0, uint val1)
{
	uint v0 = val0, v1 = val1, s0 = 0;

	[[unroll]] 
	for (uint n = 0; n < 16; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}

	return v0;
}

uint RandomInt(inout uint seed)
{
	// LCG values from Numerical Recipes
    return (seed = 1664525 * seed + 1013904223);
}

float RandomFloat(inout uint seed)
{
	//// Float version using bitmask from Numerical Recipes
	//const uint one = 0x3f800000;
	//const uint msk = 0x007fffff;
	//return uintBitsToFloat(one | (msk & (RandomInt(seed) >> 9))) - 1;

	// Faster version from NVIDIA examples; quality good enough for our use case.
	return (float(RandomInt(seed) & 0x00FFFFFF) / float(0x01000000));
}

vec2 RandomInUnitDisk(inout uint seed)
{
	// OPTIMIZATION: Polar method - no rejection sampling needed
	// Converts uniform random to unit disk without rejection loop
	const float angle = 2.0 * 3.14159265359 * RandomFloat(seed);
	const float radius = sqrt(RandomFloat(seed));
	return vec2(radius * cos(angle), radius * sin(angle));
}

// Samples a point ON the unit sphere surface — 2 RNG calls, no pow().
// For path tracing scatter directions this is equivalent to sampling inside
// the sphere and produces the same distribution of bounce directions.
vec3 RandomOnUnitSphere(inout uint seed)
{
	const float phi      = 6.28318530718 * RandomFloat(seed); // 2*pi
	const float cosTheta = 2.0 * RandomFloat(seed) - 1.0;
	const float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
	return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// Kept for API compatibility — redirects to the cheaper surface sampler.
vec3 RandomInUnitSphere(inout uint seed)
{
	return RandomOnUnitSphere(seed);
}
