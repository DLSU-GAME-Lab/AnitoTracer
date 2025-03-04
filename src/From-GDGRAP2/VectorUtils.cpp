#include "VectorUtils.h"
#include "MathUtils.h"
VectorUtils::vec3 VectorUtils::zeros()
{
	return vec3(0.0f, 0.0f, 0.0f);
}

VectorUtils::vec3 VectorUtils::ones()
{
	return vec3(1.0f, 1.0f, 1.0f);
}

VectorUtils::vec3 VectorUtils::randomFloatVec3()
{
	return vec3(MathUtils::randomFloat(), MathUtils::randomFloat(), MathUtils::randomFloat());
}

VectorUtils::vec3 VectorUtils::randomFloatVec3(float min, float max)
{
	return vec3(MathUtils::randomFloat(min, max), MathUtils::randomFloat(min, max), MathUtils::randomFloat(min, max));
}
