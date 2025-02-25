#pragma once
#include <glm/vec3.hpp>
class VectorUtils
{
public:
	typedef glm::vec3 vec3;
	static vec3 zeros();
	static vec3 ones();

	static vec3 randomFloatVec3();
	static vec3 randomFloatVec3(float min, float max);
};