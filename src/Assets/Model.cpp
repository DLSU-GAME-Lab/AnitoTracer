#include "Model.hpp"
#include "CornellBox.hpp"
#include "Box.hpp"
#include "Procedural.hpp"
#include "SphereProc.hpp"
#include "Utilities/Exception.hpp"
#include "Utilities/Console.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/hash.hpp>

#include <tiny_obj_loader.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <Assimp/postprocess.h>
#include <Assimp/texture.h>

#include "From-GDGRAP2/TextureLibrary.h"
#include "Utilities/FileUtils.h"
#include "Texture.hpp"

#include "Capsule.hpp"
#include "Cylinder.hpp"
#include "Plane.hpp"
#include "Sphere.hpp"

using namespace glm;

namespace std
{
	template<> struct hash<Assets::Vertex> final
	{
		size_t operator()(Assets::Vertex const& vertex) const noexcept
		{
			return
				Combine(hash<vec3>()(vertex.Position),
					Combine(hash<vec3>()(vertex.Normal),
						Combine(hash<vec2>()(vertex.TexCoord),
							hash<int>()(vertex.MaterialIndex))));
		}

	private:

		static size_t Combine(size_t hash0, size_t hash1)
		{
			return hash0 ^ (hash1 + 0x9e3779b9 + (hash0 << 6) + (hash0 >> 2));
		}
	};
}

namespace Assets {

	Model Model::LoadModel(const std::string& filename)
	{
		std::cout << "- loading '" << filename << "'... " << std::flush;

		const auto timer = std::chrono::high_resolution_clock::now();
		const std::string materialPath = std::filesystem::path(filename).parent_path().string();

		Assimp::Importer objectImporter;
		const aiScene* model = objectImporter.ReadFile(filename, 0);
		// read file and return an aiScene containing model attributes

		if (model == nullptr)
		{
			Throw(std::runtime_error("failed to load model '" + filename + "':\n" + objectImporter.GetErrorString()));
		}

	// Materials
	std::vector<Material> materials;

	if (model->HasMaterials()) 
	{
		for (int i = 0; i < model->mNumMaterials; i++) 
		{
			Material m{};

			aiColor4D diffuse;
			model->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
		
			//m.Diffuse.r = diffuse[0];
			//m.Diffuse.g = diffuse[1];
			//m.Diffuse.b = diffuse[2];
			//m.Diffuse.a = 1.0f;
			
			m.Diffuse = vec4(diffuse[0], diffuse[1], diffuse[2], 1.0);
		
			m.DiffuseTextureId = -1;

			materials.emplace_back(m);
		}
	}	
	else
	{
		Material m{};

		m.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0);
		m.DiffuseTextureId = -1;

		materials.emplace_back(m);
	}

		// Geometry
		std::string name;
		int totalvertices = 0;
		for (int i = 0; i < model->mNumMeshes; i++)
		{
			totalvertices += model->mMeshes[i]->mNumVertices;
		}
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::unordered_map<Vertex, uint32_t> uniqueVertices(totalvertices);
		size_t faceId = 0;

		for (int m = 0; m < model->mNumMeshes; m++)
		{
			name = model->mName.C_Str();
			for (int v = 0; v < model->mMeshes[m]->mNumVertices; v++)
			{
				Vertex vertex = {};

				vertex.Position = {
					model->mMeshes[m]->mVertices[v].x,
					model->mMeshes[m]->mVertices[v].y,
					model->mMeshes[m]->mVertices[v].z,
				};

				if (model->mMeshes[m]->HasNormals())
				{
					vertex.Normal = {
						model->mMeshes[m]->mNormals[v].x,
						model->mMeshes[m]->mNormals[v].y,
						model->mMeshes[m]->mNormals[v].z,
					};
				}
				else
				{
					// Compute a normalized vector from the vertex itself.
					auto norm = vec3(model->mMeshes[m]->mVertices[v].x,
						model->mMeshes[m]->mVertices[v].y,
						model->mMeshes[m]->mVertices[v].z);
					vertex.Normal = normalize(norm);
				}

				if (model->mMeshes[m]->HasTextureCoords(0))
				{
					vertex.TexCoord = {
						model->mMeshes[m]->mTextureCoords[0][v].x,
						1 - model->mMeshes[m]->mTextureCoords[0][v].y
					};
				}

				vertex.MaterialIndex = model->mMeshes[m]->mMaterialIndex;

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}

				indices.push_back(uniqueVertices[vertex]);
			}
		}

		// If the model did not specify normals, then create smooth normals that conserve the same number of vertices.
		if (!model->mMeshes[0]->HasNormals())
		{
			for (size_t i = 0; i < indices.size(); i += 3)
			{
				const auto normal = normalize(cross(
					vec3(vertices[indices[i + 1]].Position) - vec3(vertices[indices[i]].Position),
					vec3(vertices[indices[i + 2]].Position) - vec3(vertices[indices[i]].Position)));

				vertices[indices[i + 0]].Normal += normal;
				vertices[indices[i + 1]].Normal += normal;
				vertices[indices[i + 2]].Normal += normal;
			}
			for (auto& vertex : vertices)
			{
				vertex.Normal = normalize(vertex.Normal);
			}
		}

		// --- Centering the model at (0,0,0) ---
		// Compute bounding box (min and max points)
		vec3 minPos(FLT_MAX);
		vec3 maxPos(-FLT_MAX);
		for (const auto& vertex : vertices)
		{
			minPos = glm::min(minPos, vertex.Position);
			maxPos = glm::max(maxPos, vertex.Position);
		}
		vec3 center = (minPos + maxPos) * 0.5f;

		// Shift all vertices so that the model is centered at the origin.
		for (auto& vertex : vertices)
		{
			vertex.Position -= center;
		}
		// --- End centering ---

		const auto elapsed = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - timer).count();

		std::cout << "(" << totalvertices << " vertices, " << uniqueVertices.size() << " unique vertices, " << materials.size() << " materials) ";
		std::cout << elapsed << "s" << std::endl;

		return Model(name, std::move(vertices), std::move(indices), std::move(materials), nullptr);
	}


std::vector<Model> Model::LoadModelGroup(const std::string& filename)
{
	std::cout << "- loading '" << filename << "'... " << std::flush;

	const auto timer = std::chrono::high_resolution_clock::now();
	const std::string materialPath = std::filesystem::path(filename).parent_path().string();

	Assimp::Importer objectImporter;
	std::vector<Model> models;

	const aiScene* scene = objectImporter.ReadFile(filename, aiProcessPreset_TargetRealtime_MaxQuality); //read file and return an aiScene containing model attributes


	if (scene == nullptr)
	{
		Throw(std::runtime_error("failed to load model '" + filename + "':\n" + objectImporter.GetErrorString()));
	}
	// Geometry
	std::string name = "";
	int totalvertices = 0;
	//for (int i = 0; i < scene->mNumMeshes; i++)
	//{
	//	totalvertices += scene->mMeshes[i]->mNumVertices;
	//}

	size_t faceId = 0;
	int texlibcount = TextureLibrary::getInstance()->getTextureLibraryList().size();

	for (int m = 0; m < scene->mNumMeshes; m++)
	{
		name = scene->mMeshes[m]->mName.C_Str();
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::unordered_map<Vertex, uint32_t> uniqueVertices(scene->mMeshes[m]->mNumVertices);

		//Materials
		std::vector<Material> materials;
		aiColor4D diffuse;
		Material material{};
		if (AI_SUCCESS != scene->mMaterials[scene->mMeshes[m]->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
		{
			
			material.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0);
			material.DiffuseTextureId = -1;

			std::cout << "No Texture in Mesh!" << std::endl;
			
		}
		else 
		{
			material.Diffuse = vec4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
			material.DiffuseTextureId = -1;

			int texcount = scene->mMaterials[scene->mMeshes[m]->mMaterialIndex]->GetTextureCount(aiTextureType_DIFFUSE);

			if (texcount > 0) {
				aiString texture_file;
				scene->mMaterials[scene->mMeshes[m]->mMaterialIndex]->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texture_file);

				if (!TextureLibrary::getInstance()->doesTextureExist(scene->mMaterials[scene->mMeshes[m]->mMaterialIndex]->GetName().C_Str()))
					TextureLibrary::getInstance()->addTexture(scene->mMaterials[scene->mMeshes[m]->mMaterialIndex]->GetName().C_Str(), FileUtils::getAssetsFolderPath().generic_string() + "/models/" + texture_file.C_Str());	

				material.DiffuseTextureId = TextureLibrary::getInstance()->getTextureId(scene->mMaterials[scene->mMeshes[m]->mMaterialIndex]->GetName().C_Str());
			}

		}

		materials.emplace_back(material);

		//faces
		for (int f = 0; f < scene->mMeshes[m]->mNumFaces; f++) {

			for (int i = 0; i < scene->mMeshes[m]->mFaces[f].mNumIndices; i++)
			{

				Vertex vertex = {};
				int v = scene->mMeshes[m]->mFaces[f].mIndices[i];

				vertex.Position =
				{
					scene->mMeshes[m]->mVertices[v].x,
					scene->mMeshes[m]->mVertices[v].y,
					scene->mMeshes[m]->mVertices[v].z,
				};

				if (scene->mMeshes[m]->HasNormals())
				{
					vertex.Normal =
					{
						scene->mMeshes[m]->mNormals[v].x,
						scene->mMeshes[m]->mNormals[v].y,
						scene->mMeshes[m]->mNormals[v].z,
					};
				}
				else
				{
					vertex.Normal =
					{
						scene->mMeshes[m]->mVertices[v].Normalize().x,
						scene->mMeshes[m]->mVertices[v].Normalize().y,
						scene->mMeshes[m]->mVertices[v].Normalize().z,
					};
				}

				if (scene->mMeshes[m]->HasTextureCoords(0))
				{
					vertex.TexCoord =
					{
						(float)scene->mMeshes[m]->mTextureCoords[0][v].x,
						(float)scene->mMeshes[m]->mTextureCoords[0][v].y
					};
				}

				//vertex.MaterialIndex = std::max(0, mesh.material_ids[faceId++ / 3]);

				vertex.MaterialIndex = scene->mMeshes[m]->mMaterialIndex;

				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);

				
				indices.push_back(uniqueVertices[vertex]);
			}

		}

		//if (!scene->mMeshes[m]->HasNormals())
		//{
		//	std::vector<vec3> normals(vertices.size());

		//	for (size_t i = 0; i < indices.size(); i += 3)
		//	{
		//		const auto normal = normalize(cross(
		//			vec3(vertices[indices[i + 1]].Position) - vec3(vertices[indices[i]].Position),
		//			vec3(vertices[indices[i + 2]].Position) - vec3(vertices[indices[i]].Position)));

		//		vertices[indices[i + 0]].Normal += normal;
		//		vertices[indices[i + 1]].Normal += normal;
		//		vertices[indices[i + 2]].Normal += normal;
		//	}

		//	for (auto& vertex : vertices)
		//	{
		//		vertex.Normal = normalize(vertex.Normal);
		//	}
		//}

		if (name == "")
			name = "Sponza_" + m;

		Model model = Model(name, std::move(vertices), std::move(indices), std::move(materials), nullptr);
		models.push_back(model);
	}

	const auto elapsed = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - timer).count();

	//std::cout << "(" << totalvertices << " vertices, " << uniqueVertices.size() << " unique vertices, " << materials.size() << " materials) ";
	//std::cout << elapsed << "s" << std::endl;

	objectImporter.FreeScene();
	return models;
}


Model Model::CreateCornellBox(const float scale)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<Material> materials;

	CornellBox::Create(scale, vertices, indices, materials);

	return Model("CornellBox",
		std::move(vertices),
		std::move(indices),
		std::move(materials),
		nullptr
	);
}

Model Model::CreateBox(const vec3& p0, const vec3& p1, const Material& material)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Box::Create(p0, p1, vertices, indices);

	return Model("Box",
		std::move(vertices),
		std::move(indices),
		std::vector<Material>{material},
		nullptr);
}

Model Model::CreatePlane(const glm::vec3& p0, const glm::vec3& p1, const Material& material)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Plane::Create(p0, p1, vertices, indices);

	return Model("Plane",
		std::move(vertices),
		std::move(indices),
		std::vector<Material>{material},
		nullptr);
}

Model Model::CreateSphere(const vec3& center, float radius, const Material& material, const bool isProcedural)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Sphere::Create(center, radius, vertices, indices);

	return Model("Sphere",
		std::move(vertices),
		std::move(indices),
		std::vector<Material>{material},
		isProcedural ? new SphereProc(center, radius) : nullptr);
}

Model Model::CreateCylinder(float radius, float height, const Material& material)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Assets::Cylinder::Create(vec3(0, 0, 0), radius, height, vertices, indices);

	return Model("Cylinder",
		std::move(vertices),
		std::move(indices),
		std::vector<Material>{material},
		nullptr);
}

Model Model::CreateCapsule(float radius, float height, const Material& material)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Assets::Capsule::Create(vec3(0, 0, 0), radius, height, vertices, indices);

	return Model("Capsule",
		std::move(vertices),
		std::move(indices),
		std::vector<Material>{material},
		nullptr);
}

void Model::SetMaterial(const Material& material)
{
	if (materials_.size() != 1)
	{
		Throw(std::runtime_error("cannot change material on a multi-material model"));
	}

	materials_[0] = material;
}

void Model::Transform(const mat4& transform)
{
	const auto transformIT = inverseTranspose(transform);

	for (auto& vertex : vertices_)
	{
		vertex.Position = transform * vec4(vertex.Position, 1);
		vertex.Normal = transformIT * vec4(vertex.Normal, 0);
	}
}

Model::Model(std::string name, std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices, std::vector<Material>&& materials, const class Procedural* procedural) :
	vertices_(std::move(vertices)), 
	indices_(std::move(indices)),
	materials_(std::move(materials)),
	procedural_(procedural)

{
	this->name = name;
}

}
