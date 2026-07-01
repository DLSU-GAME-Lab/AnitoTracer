#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Mat44.h>

namespace PhysicsUtils
{
	inline void PrintVec3(const JPH::RVec3 vec) {
		std::cout << "(" << vec.GetX() << ", " << vec.GetY() << ", " << vec.GetZ() << ")";
	}

	/// Convert glm::vec3 to Jolt::RVec3
	/// @param vec The glm vector to convert
	/// @return Converted Jolt vector
	inline JPH::RVec3 ToJoltVec3(const glm::vec3& vec)
	{
		return JPH::RVec3(vec.x, vec.y, vec.z);
	}

	/// Convert Jolt::RVec3 to glm::vec3
	/// @param vec The Jolt vector to convert
	/// @return Converted glm vector
	inline glm::vec3 ToGlmVec3(const JPH::RVec3& vec)
	{
		return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ());
	}

	/// Convert glm::vec3 to Jolt::Vec3
	/// @param vec The glm vector to convert
	/// @return Converted Jolt vector
	inline JPH::Vec3 ToJoltVec3Float(const glm::vec3& vec)
	{
		return JPH::Vec3(vec.x, vec.y, vec.z);
	}

	/// Convert Jolt::Vec3 to glm::vec3
	/// @param vec The Jolt vector to convert
	/// @return Converted glm vector
	inline glm::vec3 ToGlmVec3Float(const JPH::Vec3& vec)
	{
		return glm::vec3(vec.GetX(), vec.GetY(), vec.GetZ());
	}

	/// Convert glm::quat to Jolt::Quat
	/// @param quat The glm quaternion to convert
	/// @return Converted Jolt quaternion
	inline JPH::Quat ToJoltQuat(const glm::quat& quat)
	{
		return JPH::Quat(quat.x, quat.y, quat.z, quat.w);
	}

	/// Convert Jolt::Quat to glm::quat
	/// @param quat The Jolt quaternion to convert
	/// @return Converted glm quaternion
	inline glm::quat ToGlmQuat(const JPH::Quat& quat)
	{
		return glm::quat(quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ());
	}

	/// Convert glm::mat4 to Jolt RMat44
	/// @param mat The glm matrix to convert
	/// @return Converted Jolt matrix
	inline JPH::RMat44 ToJoltMat44(const glm::mat4& mat)
	{
		// GLM stores matrices column-major. 
		// We can initialize JPH::Vec4 columns directly from GLM vec4 columns.
		JPH::Vec4 col0(mat[0].x, mat[0].y, mat[0].z, mat[0].w);
		JPH::Vec4 col1(mat[1].x, mat[1].y, mat[1].z, mat[1].w);
		JPH::Vec4 col2(mat[2].x, mat[2].y, mat[2].z, mat[2].w);

		// The 4th column is translation (handled as an RVec3 in RMat44)
		JPH::RVec3 col3(mat[3].x, mat[3].y, mat[3].z);

		return JPH::RMat44(col0, col1, col2, col3);
	}

	/// Convert Jolt RMat44 to glm::mat4
	/// @param mat The Jolt matrix to convert
	/// @return Converted glm matrix
	inline glm::mat4 ToGlmMat4(const JPH::RMat44& mat)
	{
		glm::mat4 result(1.0f);

		// Extract translation
		JPH::RVec3 translation = mat.GetTranslation();
		result[3] = glm::vec4(ToGlmVec3(translation), 1.0f);

		// Extract rotation/scale
		JPH::Mat44 rotMatPart = JPH::Mat44(mat.GetRotation());
		for (int i = 0; i < 3; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				result[i][j] = rotMatPart(j, i);
			}
		}

		return result;
	}
}
