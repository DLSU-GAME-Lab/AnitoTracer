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
#include "RayTracer.hpp"
#include "Sphere.hpp"
#include "Assets/ModelLibrary.hpp"
#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/GameObject.h"

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
		const aiScene* scene = objectImporter.ReadFile(filename, aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality | aiProcess_LimitBoneWeights | aiProcess_SplitLargeMeshes | aiProcess_Triangulate |
			aiProcess_GenUVCoords | aiProcess_SortByPType | aiProcess_FindInvalidData | aiProcess_ValidateDataStructure | aiProcess_FlipUVs | 0);
		// read file and return an aiScene containing model attributes

		if (scene == nullptr)
		{
			Throw(std::runtime_error("failed to load model '" + filename + "':\n" + objectImporter.GetErrorString()));
		}
		std::string name;
		int totalvertices = 0;

		for (int i = 0; i < scene->mNumMeshes; i++)
		{
			totalvertices += scene->mMeshes[i]->mNumVertices;
		}
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::unordered_map<Vertex, uint32_t> uniqueVertices(totalvertices);
		size_t faceId = 0;

		//instantiate all materials 
		//Materials and Texture
		std::vector<Material> materials;
		aiColor4D diffuse;
		Material material{};
		for (int i = 0; i < scene->mNumMaterials; i++)
		{
			if (AI_SUCCESS != scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
			{

				material.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0);
				material.DiffuseTextureId = -1;

				std::cout << "No Texture in Mesh!" << std::endl;

			}
			else
			{
				material.Diffuse = vec4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);

				//diffuse/albedo
				int texcount = scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE);

				if (texcount > 0) {
					aiString texture_file;
					scene->mMaterials[i]->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texture_file);
					std::string texName = scene->mMaterials[i]->GetName().C_Str();
					if (!TextureLibrary::getInstance()->doesTextureExist(texName))
					{
						TextureLibrary::getInstance()->addTexture(texName, materialPath + "/" + texture_file.C_Str());
						std::cout << "Initialized Texture " << texName << std::endl;
					}

					material.DiffuseTextureId = TextureLibrary::getInstance()->getTextureId(texName);

				}
				else
				{
					//material.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0);
					material.DiffuseTextureId = -1;
				}


				////Normal Map
				//texcount = scene->mMaterials[i]->GetTextureCount(aiTextureType_NORMALS);

				//if (texcount > 0) {
				//	aiString texture_file;
				//	scene->mMaterials[i]->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), texture_file);
				//	std::string texName = scene->mMaterials[i]->GetName().C_Str();
				//	if (!TextureLibrary::getInstance()->doesTextureExist(texName))
				//	{
				//		TextureLibrary::getInstance()->addTexture(texName, materialPath + "/" + texture_file.C_Str());
				//		std::cout << "Initialized Texture " << texName << std::endl;
				//	}

				//	material.NormalTextureId = TextureLibrary::getInstance()->getTextureId(texName);

				//}
				//else
				//{
				//	//material.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0);
				//	material.NormalTextureId = -1;
				//}

				////Metallic Map
				//texcount = scene->mMaterials[i]->GetTextureCount(aiTextureType_METALNESS);

				//if (texcount > 0) {
				//	aiString texture_file;
				//	scene->mMaterials[i]->Get(AI_MATKEY_TEXTURE(aiTextureType_METALNESS, 0), texture_file);
				//	std::string texName = scene->mMaterials[i]->GetName().C_Str();
				//	if (!TextureLibrary::getInstance()->doesTextureExist(texName))
				//	{
				//		TextureLibrary::getInstance()->addTexture(texName, materialPath + "/" + texture_file.C_Str());
				//		std::cout << "Initialized Texture " << texName << std::endl;
				//	}

				//	material.MetallicTextureId = TextureLibrary::getInstance()->getTextureId(texName);

				//}
				//else
				//{
				//	//material.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0);
				//	material.MetallicTextureId = -1;
				//}


				////Metallic Map
				//texcount = scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS);

				//if (texcount > 0) {
				//	aiString texture_file;
				//	scene->mMaterials[i]->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE_ROUGHNESS, 0), texture_file);
				//	std::string texName = scene->mMaterials[i]->GetName().C_Str();
				//	if (!TextureLibrary::getInstance()->doesTextureExist(texName))
				//	{
				//		TextureLibrary::getInstance()->addTexture(texName, materialPath + "/" + texture_file.C_Str());
				//		std::cout << "Initialized Texture " << texName << std::endl;
				//	}

				//	material.RoughTextureId = TextureLibrary::getInstance()->getTextureId(texName);

				//}
				//else
				//{
				//	//material.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0);
				//	material.RoughTextureId = -1;
				//}
			}

			materials.emplace_back(material);
		}

		for (int m = 0; m < scene->mNumMeshes; m++)
		{

			// Geometry
			for (int f = 0; f < scene->mMeshes[m]->mNumFaces; f++)
			{

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
			name = scene->mName.C_Str();
			if (name == "")
				name = "Imported Object";
		}


		//// --- Centering the model at (0,0,0) ---
		//// Compute bounding box (min and max points)
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
		//// --- End centering ---

		const auto elapsed = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - timer).count();

		std::cout << "(" << totalvertices << " vertices, " << uniqueVertices.size() << " unique vertices, " << materials.size() << " materials) ";
		std::cout << elapsed << "s" << std::endl;

		Model model = Model(name, std::move(vertices), std::move(indices), std::move(materials), nullptr);
		model.filepath = filename;

		return model;
	}


	std::vector<Model> Model::LoadModelGroup(const std::string& filename)
	{
		std::cout << "- loading '" << filename << "'... " << std::flush;

		const auto timer = std::chrono::high_resolution_clock::now();
		const std::string materialPath = std::filesystem::path(filename).parent_path().string();

		Assimp::Importer objectImporter;
		std::vector<Model> models;

		const aiScene* scene = objectImporter.ReadFile(filename, aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality | aiProcess_LimitBoneWeights | aiProcess_SplitLargeMeshes | aiProcess_Triangulate |
			aiProcess_GenUVCoords | aiProcess_SortByPType | aiProcess_FindInvalidData | aiProcess_ValidateDataStructure | aiProcess_FlipUVs | 0); //read file and return an aiScene containing model attributes


		if (scene == nullptr)
		{
			Throw(std::runtime_error("failed to load model '" + filename + "':\n" + objectImporter.GetErrorString()));
		}

		std::string name = "";
		int totalvertices = 0;
		size_t faceId = 0;
		int texlibcount = TextureLibrary::getInstance()->getTextureLibraryList().size() - 1;


		//instantiate all materials 
		//Materials and Texture
		std::vector<Material> materials;
		aiColor4D diffuse;
		Material material{};
		for (int i = 0; i < scene->mNumMaterials; i++) 
		{
			if (AI_SUCCESS != scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
			{

				material.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0);
				material.DiffuseTextureId = -1;

				std::cout << "No Texture in Mesh!" << std::endl;

			}
			else
			{
				material.Diffuse = vec4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);

				int texcount = scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE);

				if (texcount > 0) {
					aiString texture_file;
					scene->mMaterials[i]->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0), texture_file);
					std::string texName = scene->mMaterials[i]->GetName().C_Str();
					if (!TextureLibrary::getInstance()->doesTextureExist(texName))
					{
						TextureLibrary::getInstance()->addTexture(texName, materialPath + "/" + texture_file.C_Str());
						std::cout << "Initialized Texture " << texName << std::endl;
					}

					material.DiffuseTextureId = TextureLibrary::getInstance()->getTextureId(texName);

				}
				else
				{
					//material.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0);
					material.DiffuseTextureId = -1;
				}


			}

			materials.emplace_back(material);
		}

		for (int m = 0; m < scene->mNumMeshes; m++)
		{
			name = scene->mMeshes[m]->mName.C_Str();
			std::vector<Material> meshMaterials = materials;
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			std::unordered_map<Vertex, uint32_t> uniqueVertices(scene->mMeshes[m]->mNumVertices);

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

			if (name == "")
				name = "Object_" + m;

			//// --- Centering the model at (0,0,0) ---
			//// Compute bounding box (min and max points)
			//vec3 minPos(FLT_MAX);
			//vec3 maxPos(-FLT_MAX);
			//for (const auto& vertex : vertices)
			//{
			//	minPos = glm::min(minPos, vertex.Position);
			//	maxPos = glm::max(maxPos, vertex.Position);
			//}
			//vec3 center = (minPos + maxPos) * 0.5f;

			//// Shift all vertices so that the model is centered at the origin.
			//for (auto& vertex : vertices)
			//{
			//	vertex.Position -= center;
			//}
			//// --- End centering ---


			Model model = Model(name, std::move(vertices), std::move(indices), std::move(meshMaterials), nullptr);
			model.filepath = filename;
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
		nullptr
		);
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
		nullptr
		);
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
		isProcedural ? new SphereProc(center, radius) : nullptr
		);
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
		nullptr
	);
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
		nullptr
		);
}

void Model::SetMaterial(const Material& material)
{
	if (materials_.size() != 1)
	{
		Throw(std::runtime_error("cannot change material on a multi-material model"));
	}

	materials_[0] = material;
}

void Model::SetMaterials(std::vector<Material> mats)
{
	this->materials_ = mats;
}

void Model::SetMaterialIndex(int index)
{
	this->materials_[0].DiffuseTextureId = index;
}

void Model::Transform(const mat4& transform)
{
	worldMatrix_ = transform;
	if (RayTracer::getInstance()->getUserSettings().IsRayTraced)
	{
		const auto transformIT = inverseTranspose(transform);
		for (size_t i = 0; i < vertices_.size(); i++)
		{
			transformedVertices_[i].Position = transform * vec4(originalVertices_[i].Position, 1);
			transformedVertices_[i].Normal = transformIT * vec4(originalVertices_[i].Normal, 0);
			vertices_[i].Position = transformedVertices_[i].Position;
			vertices_[i].Normal = transformedVertices_[i].Normal;
		}
	}
	
}

void Model::ResetVertices()
{
	if (RayTracer::getInstance()->getUserSettings().IsRayTraced)
	{
		this->Transform(this->worldMatrix_);
	}
	else
	{
		for (size_t i = 0; i < vertices_.size(); i++)
		{
			vertices_[i].Position = originalVertices_[i].Position;
			vertices_[i].Normal = originalVertices_[i].Normal;
		}
	}
}

GameObject* Model::GetOwner() const
{
	if (!this->owner) Debug::Log("OWNER LESS MODEL DETECTED!"); 
	return this->owner;
}

Model::Model(std::string name, std::vector<Vertex>&& vertices, std::vector<uint32_t>&& indices, std::vector<Material>&& materials, const class Procedural* procedural) :
	vertices_(std::move(vertices)), 
	indices_(std::move(indices)),
	materials_(std::move(materials)),
	procedural_(procedural)

{
	this->name = name;
	this->originalVertices_ = this->vertices_;
	this->transformedVertices_ = this->vertices_;
	this->worldMatrix_ = mat4(1.0f);
}

std::shared_ptr<Model> Model::Clone() const
{
	return std::make_shared<Model>(
		this->name,
		std::vector<Vertex>(this->vertices_),
		std::vector<uint32_t>(this->indices_),
		std::vector<Material>(this->materials_),
		nullptr
	);
}

void Model::SetName(std::string name)
{
	this->name = name;
}
}
