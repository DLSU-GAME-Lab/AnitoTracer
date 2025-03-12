#pragma once

#include "Material.hpp"
#include "Procedural.hpp"
#include "Vertex.hpp"
#include <string>
#include <memory>
#include <string>
#include <vector>

namespace Assets
{
	class Model final
	{
	public:
		static Model LoadModel(const std::string& filename);
		static std::vector<Model> LoadModelGroup(const std::string& filename);

		//static Model LoadModelFromFile(const std::string& filename);
		//static std::vector<Model> LoadModelGroupFromFile(const std::string& filename);

		static Model CreateCornellBox(const float scale);
		static Model CreateBox(const glm::vec3& p0, const glm::vec3& p1, const Material& material);
		static Model CreatePlane(const glm::vec3& p0, const glm::vec3& p1, const Material& material);
		static Model CreateSphere(const glm::vec3& center, float radius, const Material& material, bool isProcedural);
		static Model CreateCylinder(float radius, float height, const Material& material);
		static Model CreateCapsule(float radius, float height, const Material& material);
		
		Model& operator = (const Model&) = delete;
		Model& operator = (Model&&) = delete;

		Model() = default;
		Model(const Model&) = default;
		Model(Model&&) = default;
		~Model() = default;
		Model(std::string name, std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices, std::vector<Material>&& materials, const class Procedural* procedural);

		void SetMaterial(const Material& material);
		void SetMaterialIndex(int index);
		void Transform(const glm::mat4& transform);

		const std::vector<Vertex>& Vertices() const { return vertices_; }
		const std::vector<uint32_t>& Indices() const { return indices_; }
		const std::vector<Material>& Materials() const { return materials_; }

		Material* getMaterial(const unsigned index)
		{
			if (materials_.empty() || index >= materials_.size())
				return nullptr;
				
			return &materials_.at(index);
		}

		const class Procedural* Procedural() const { return procedural_.get(); }

		uint32_t NumberOfVertices() const { return static_cast<uint32_t>(vertices_.size()); }
		uint32_t NumberOfIndices() const { return static_cast<uint32_t>(indices_.size()); }
		uint32_t NumberOfMaterials() const { return static_cast<uint32_t>(materials_.size()); }
		std::string GetName() const { return name; }

	public:

		std::string name;
		std::vector<Vertex> vertices_;
		std::vector<uint32_t> indices_;
		std::vector<Material> materials_;
		std::shared_ptr<const class Procedural> procedural_;
	};

}
