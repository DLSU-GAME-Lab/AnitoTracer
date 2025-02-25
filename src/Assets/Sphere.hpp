#pragma once

#include "Material.hpp"
#include "Vertex.hpp"
#include <vector>

namespace Assets
{
	class Sphere final
	{
	public:

		static void Create(const glm::vec3& center, float radius, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

	};

}
