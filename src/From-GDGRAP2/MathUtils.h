#pragma once
#include <cmath>
#include <limits>
#include <memory>
#include <glm/vec3.hpp>

class MathUtils
{
public:
	static float infinity();
	static float degreesToRadians(float degrees);
	static float randomFloat();
	static float randomFloat(float min, float max);
	
	static int randomInt(int min, int max, bool exclusive = false);
};

