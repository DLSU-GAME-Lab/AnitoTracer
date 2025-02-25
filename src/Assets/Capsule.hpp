#pragma once

#include "Material.hpp"
#include "Vertex.hpp"
#include <vector>

namespace Assets
{
	class Capsule final
	{
	public:

		static void Create(const glm::vec3& center, float radius, float height, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

	};

}
