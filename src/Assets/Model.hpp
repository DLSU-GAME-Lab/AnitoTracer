#pragma once

#include "Material.hpp"
#include "Procedural.hpp"
#include "Vertex.hpp"
#include <string>
#include <memory>
#include <vector>

class GameObject;

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

		virtual std::shared_ptr<Model> Clone() const;

		void SetName(std::string name);
		void SetMaterial(const Material& material);
		void SetMaterials(std::vector<Material> mats);
		void SetMaterialIndex(int index);
		void Transform(const glm::mat4& transform);
		void ResetVertices();

		void SetOwner(GameObject* owner) { this->owner = owner; }
		GameObject* GetOwner() const;

		const std::vector<Vertex>& Vertices() const { return vertices_; }
		const std::vector<uint32_t>& Indices() const { return indices_; }
		const std::vector<Material>& Materials() const { return materials_; }
		const std::vector<Vertex>& OriginalVertices() const { return originalVertices_; }
		const std::vector<Vertex>& TransformedVertices() const { return transformedVertices_; }

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
		glm::mat4 GetWorldMatrix() const { return worldMatrix_; };
		std::string FilePath() const { return filepath; }

		void SetOrigin(glm::vec3 origin) { this->origin = origin; }
		glm::vec3 GetOrigin() { return this->origin; }

	public:

		std::string name;
		std::vector<Vertex> originalVertices_;
		std::vector<Vertex> transformedVertices_;
		std::vector<Vertex> vertices_;
		std::vector<uint32_t> indices_;
		std::vector<Material> materials_;
		std::shared_ptr<const class Procedural> procedural_;
		glm::mat4 worldMatrix_;
		std::string filepath;
		glm::vec3 origin;

	private:
		GameObject* owner = nullptr;
	};

}
