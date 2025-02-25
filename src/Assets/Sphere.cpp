#include "Sphere.hpp"

#include "Model.hpp"

using namespace glm;

void Assets::Sphere::Create(const glm::vec3& center, float radius, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
	const int slices = 32;
	const int stacks = 16;

	const float pi = 3.14159265358979f;

	for (int j = 0; j <= stacks; ++j)
	{
		const float j0 = pi * j / stacks;

		// Vertex
		const float v = radius * -std::sin(j0);
		const float z = radius * std::cos(j0);

		// Normals		
		const float n0 = -std::sin(j0);
		const float n1 = std::cos(j0);

		for (int i = 0; i <= slices; ++i)
		{
			const float i0 = 2 * pi * i / slices;

			const vec3 position(
				center.x + v * std::sin(i0),
				center.y + z,
				center.z + v * std::cos(i0));

			const vec3 normal(
				n0 * std::sin(i0),
				n1,
				n0 * std::cos(i0));

			const vec2 texCoord(
				static_cast<float>(i) / slices,
				static_cast<float>(j) / stacks);

			vertices.push_back(Vertex{ position, normal, texCoord, 0 });
		}
	}

	for (int j = 0; j < stacks; ++j)
	{
		for (int i = 0; i < slices; ++i)
		{
			const auto j0 = (j + 0) * (slices + 1);
			const auto j1 = (j + 1) * (slices + 1);
			const auto i0 = i + 0;
			const auto i1 = i + 1;

			indices.push_back(j0 + i0);
			indices.push_back(j1 + i0);
			indices.push_back(j1 + i1);

			indices.push_back(j0 + i0);
			indices.push_back(j1 + i1);
			indices.push_back(j0 + i1);
		}
	}
}
