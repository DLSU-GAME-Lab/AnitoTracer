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

#include "Engine/Scene/SceneIO.hpp"

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

		//Import the model to the project folder first and get the path for saving
		std::string path = SceneIO::getInstance()->CopyToProjectFolder(filename, SceneIO::FILETYPE::MODEL);

		std::cout << "- loading '" << filename << "'... " << std::flush;
		
		const auto timer = std::chrono::high_resolution_clock::now();
		const std::string materialPath =
			std::filesystem::path(filename).parent_path().string();

		Assimp::Importer importer;

		//Use the original path for importing still
		// Keep postprocess flags conservative first
		const aiScene* scene = importer.ReadFile(
			filename,
			aiProcess_Triangulate |
			aiProcess_GenSmoothNormals |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_FlipUVs
		);

		if (!scene || !scene->HasMeshes())
		{
			Throw(std::runtime_error(
				"failed to load model '" + filename + "':\n" +
				importer.GetErrorString()));
		}

		std::string name = scene->mName.C_Str();
		if (name.empty())
			name = "Imported Object";

		// Materials
		std::vector<Material> materials;

		for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
		{
			Material material{};

			aiMaterial* aiMat = scene->mMaterials[i];

			aiColor4D diffuse;
			if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
			{
				material.Diffuse =
				{
					diffuse.r,
					diffuse.g,
					diffuse.b,
					diffuse.a
				};
			}
			else
			{
				material.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0f);
			}

			material.DiffuseTextureId = -1;

			if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			{
				aiString textureFile;

				if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &textureFile) == AI_SUCCESS)
				{
					std::string texPath = materialPath + "/" + textureFile.C_Str();
					std::string texName = textureFile.C_Str();

					if (!TextureLibrary::getInstance()->doesTextureExist(texName))
					{
						TextureLibrary::getInstance()->addTexture(texName, texPath);
						std::cout << "Initialized Texture " << texName << std::endl;
					}

					material.DiffuseTextureId =
						TextureLibrary::getInstance()->getTextureId(texName);
				}
			}

			materials.push_back(material);
		}

		// Geometry
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		vertices.reserve(100000);
		indices.reserve(100000);

		uint32_t baseVertex = 0;

		for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
		{
			aiMesh* mesh = scene->mMeshes[m];

			// Copy vertices ONCE
			for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
			{
				Vertex vertex{};

				// Position
				vertex.Position =
				{
					mesh->mVertices[v].x,
					mesh->mVertices[v].y,
					mesh->mVertices[v].z
				};

				// Normal
				if (mesh->HasNormals())
				{
					vertex.Normal =
					{
						mesh->mNormals[v].x,
						mesh->mNormals[v].y,
						mesh->mNormals[v].z
					};
				}
				else
				{
					vertex.Normal = vec3(0.0f, 1.0f, 0.0f);
				}

				// UV
				if (mesh->HasTextureCoords(0))
				{
					vertex.TexCoord =
					{
						mesh->mTextureCoords[0][v].x,
						mesh->mTextureCoords[0][v].y
					};
				}
				else
				{
					vertex.TexCoord = vec2(0.0f);
				}

				// Material
				vertex.MaterialIndex = mesh->mMaterialIndex;

				vertices.push_back(vertex);
			}

			// Copy indices directly
			for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
			{
				const aiFace& face = mesh->mFaces[f];

				// aiProcess_Triangulate guarantees triangles
				if (face.mNumIndices != 3)
					continue;

				indices.push_back(baseVertex + face.mIndices[0]);
				indices.push_back(baseVertex + face.mIndices[1]);
				indices.push_back(baseVertex + face.mIndices[2]);
			}

			baseVertex += mesh->mNumVertices;
		}

		// Center model
		vec3 minPos(FLT_MAX);
		vec3 maxPos(-FLT_MAX);

		for (const auto& vertex : vertices)
		{
			minPos = glm::min(minPos, vertex.Position);
			maxPos = glm::max(maxPos, vertex.Position);
		}

		vec3 center = (minPos + maxPos) * 0.5f;

		for (auto& vertex : vertices)
		{
			vertex.Position -= center;
		}

		// Debug info
		const auto elapsed =
			std::chrono::duration<float>(
				std::chrono::high_resolution_clock::now() - timer
			).count();

		std::cout
			<< vertices.size() << " verts, "
			<< indices.size() / 3 << " tris, "
			<< materials.size() << " mats, "
			<< elapsed << "s"
			<< std::endl;

		// Create model
		Model model(
			name,
			std::move(vertices),
			std::move(indices),
			std::move(materials),
			nullptr
		);

		model.filepath = path;

		return model;
	}

	std::vector<Model> Model::LoadModelGroup(const std::string& filename)
	{
		std::string path = SceneIO::getInstance()->CopyToProjectFolder(filename, SceneIO::FILETYPE::MODEL);

		std::cout << "- loading '" << path << "'... " << std::flush;

		const auto timer = std::chrono::high_resolution_clock::now();
		const std::string materialPath =
			std::filesystem::path(path).parent_path().string();
		std::vector<Model> models;

		Assimp::Importer importer;

		// Keep postprocess flags conservative first
		const aiScene* scene = importer.ReadFile(
			path,
			aiProcess_Triangulate |
			aiProcess_GenSmoothNormals |
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_FlipUVs
		);

		if (!scene || !scene->HasMeshes())
		{
			Throw(std::runtime_error(
				"failed to load model '" + path + "':\n" +
				importer.GetErrorString()));
		}

		// Materials
		std::vector<Material> materials;

		for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
		{
			Material material{};

			aiMaterial* aiMat = scene->mMaterials[i];

			aiColor4D diffuse;
			if (AI_SUCCESS == aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
			{
				material.Diffuse =
				{
					diffuse.r,
					diffuse.g,
					diffuse.b,
					diffuse.a
				};
			}
			else
			{
				material.Diffuse = vec4(0.7f, 0.7f, 0.7f, 1.0f);
			}

			material.DiffuseTextureId = -1;

			if (aiMat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			{
				aiString textureFile;

				if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &textureFile) == AI_SUCCESS)
				{
					std::string texPath = materialPath + "/" + textureFile.C_Str();
					std::string texName = textureFile.C_Str();

					if (!TextureLibrary::getInstance()->doesTextureExist(texName))
					{
						TextureLibrary::getInstance()->addTexture(texName, texPath);
						std::cout << "Initialized Texture " << texName << std::endl;
					}

					material.DiffuseTextureId =
						TextureLibrary::getInstance()->getTextureId(texName);
				}
			}

			materials.push_back(material);
		}

		// Geometry
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		vertices.reserve(100000);
		indices.reserve(100000);

		uint32_t baseVertex = 0;

		for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
		{
			aiMesh* mesh = scene->mMeshes[m];

			std::string name = scene->mName.C_Str();
			if (name.empty())
				name = "Imported Object";
			else
				name += "_" + std::to_string(m);
		

			// Copy vertices ONCE
			for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
			{
				Vertex vertex{};

				// Position
				vertex.Position =
				{
					mesh->mVertices[v].x,
					mesh->mVertices[v].y,
					mesh->mVertices[v].z
				};

				// Normal
				if (mesh->HasNormals())
				{
					vertex.Normal =
					{
						mesh->mNormals[v].x,
						mesh->mNormals[v].y,
						mesh->mNormals[v].z
					};
				}
				else
				{
					vertex.Normal = vec3(0.0f, 1.0f, 0.0f);
				}

				// UV
				if (mesh->HasTextureCoords(0))
				{
					vertex.TexCoord =
					{
						mesh->mTextureCoords[0][v].x,
						mesh->mTextureCoords[0][v].y
					};
				}
				else
				{
					vertex.TexCoord = vec2(0.0f);
				}

				// Material
				vertex.MaterialIndex = mesh->mMaterialIndex;

				vertices.push_back(vertex);
			}

			// Copy indices directly
			for (unsigned int f = 0; f < mesh->mNumFaces; ++f)
			{
				const aiFace& face = mesh->mFaces[f];

				// aiProcess_Triangulate guarantees triangles
				if (face.mNumIndices != 3)
					continue;

				indices.push_back(baseVertex + face.mIndices[0]);
				indices.push_back(baseVertex + face.mIndices[1]);
				indices.push_back(baseVertex + face.mIndices[2]);
			}

			baseVertex += mesh->mNumVertices;

			// Create model
			Model model(
				name,
				std::move(vertices),
				std::move(indices),
				std::move(materials),
				nullptr
			);

			model.filepath = path;

			models.push_back(std::move(model));
		}

		// Debug info
		const auto elapsed =
			std::chrono::duration<float>(
				std::chrono::high_resolution_clock::now() - timer
			).count();

		std::cout
			<< models.size() << " models, "
			<< vertices.size() << " verts, "
			<< indices.size() / 3 << " tris, "
			<< materials.size() << " mats, "
			<< elapsed << "s"
			<< std::endl;

		importer.FreeScene();
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

int Model::GetMaterialIndex()
{
	return this->materials_[0].DiffuseTextureId;
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
