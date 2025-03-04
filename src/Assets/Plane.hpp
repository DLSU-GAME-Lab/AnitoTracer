#pragma once

#include "Material.hpp"
#include "Vertex.hpp"
#include <vector>

namespace Assets
{
	class Plane final
	{
	public:

		static void Create(
			const glm::vec3& p0, const glm::vec3& p1,
			std::vector<Vertex>& vertices,
			std::vector<uint32_t>& indices);

	};

}
