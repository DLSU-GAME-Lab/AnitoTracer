#include "ModelLibrary.hpp"

/* Default Models */
#include "Box.hpp"
#include "Plane.hpp"
#include "Capsule.hpp"
#include "Cylinder.hpp"
#include "Sphere.hpp"
#include "SphereProc.hpp"
#include "CornellBox.hpp"

#include "Utilities/FileUtils.h"
#include "Procedural.hpp"
#include "From-GDGRAP2/MaterialLibrary.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <Assimp/postprocess.h>
#include <Assimp/texture.h>

#include "From-GDGRAP2/Debug.h"
#include "From-GDGRAP2/TextureLibrary.h"

using namespace glm;

namespace std
{
	template<> struct hash<vec3> final
	{
		size_t operator()(vec3 const& v) const noexcept
		{
			const size_t h1 = hash<float>()(v.x);
			const size_t h2 = hash<float>()(v.y);
			const size_t h3 = hash<float>()(v.z);
			return Combine(Combine(h1, h2), h3);
		}

	private:
		static size_t Combine(size_t hash0, size_t hash1)
		{
			return hash0 ^ (hash1 + 0x9e3779b9 + (hash0 << 6) + (hash0 >> 2));
		}
	};

	template<> struct hash<vec2> final
	{
		size_t operator()(vec2 const& v) const noexcept
		{
			const size_t h1 = hash<float>()(v.x);
			const size_t h2 = hash<float>()(v.y);
			return Combine(h1, h2);
		}

	private:
		static size_t Combine(size_t hash0, size_t hash1)
		{
			return hash0 ^ (hash1 + 0x9e3779b9 + (hash0 << 6) + (hash0 >> 2));
		}
	};

	// Existing Vertex has operator==; provide a hash specialization that composes
	// the pre-defined glm vector hashes and the material index.
	template<> struct hash<Assets::Vertex> final
	{
		size_t operator()(Assets::Vertex const& vertex) const noexcept
		{
			return
				Combine(hash<vec3>()(vertex.Position),
					Combine(hash<vec3>()(vertex.Normal),
						Combine(hash<vec2>()(vertex.TexCoord),
							hash<int32_t>()(vertex.MaterialIndex))));
		}

	private:

		static size_t Combine(size_t hash0, size_t hash1)
		{
			return hash0 ^ (hash1 + 0x9e3779b9 + (hash0 << 6) + (hash0 >> 2));
		}
	};
}

Assets::ModelLibrary::ModelLibrary()
{
	this->defaultMat = MaterialLibrary::getInstance()->getMaterial(L"White");
	LoadInitialModels();
}

Assets::ModelLibrary::ModelPtr Assets::ModelLibrary::GetModel(const String& meshName)
{
	ModelPtr result = nullptr;

	auto it = this->m_meshMap.find(meshName);

	if (it != this->m_meshMap.end())
	{
		result = std::make_shared<Model>(*(it->second)); // Due to how the Model is used, we need to return a copy
	}

	if(result == nullptr)
	{
		Debug::Log("Model: '" + meshName + "' not found in ModelLibrary.");
	}

	return result;
}

void Assets::ModelLibrary::LoadInitialModels()
{
	/* Primitives */
	this->m_meshMap.insert({ "CUBE", std::move(this->LoadBox()) });
	this->m_meshMap.insert({ "PLANE", std::move(this->LoadPlane()) });
	this->m_meshMap.insert({ "SPHERE", std::move(this->LoadSphere()) });
	this->m_meshMap.insert({ "CYLINDER", std::move(this->LoadCylinder()) });
	this->m_meshMap.insert({ "CAPSULE", std::move(this->LoadCapsule()) });
	this->m_meshMap.insert({ "CORNELL_BOX", std::move(this->LoadCornellBox()) });
}

Assets::ModelLibrary::ModelPtr Assets::ModelLibrary::LoadBox()
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Box::Create(this->m_cube_p0, this->m_cube_p1, vertices, indices);

	ModelPtr model = std::make_shared<Model>("Box",
		std::move(vertices),
		std::move(indices),
		std::vector<Material>{ *this->defaultMat },
		nullptr);

	return std::move(model);
}

Assets::ModelLibrary::ModelPtr  Assets::ModelLibrary::LoadPlane()
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Box::Create(this->m_plane_p0, this->m_plane_p1, vertices, indices);

	ModelPtr model = std::make_shared<Model>("Plane",
		std::move(vertices),
		std::move(indices),
		std::vector<Material>{ *this->defaultMat },
		nullptr);

	return std::move(model);
}

Assets::ModelLibrary::ModelPtr  Assets::ModelLibrary::LoadSphere()
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Sphere::Create(this->m_sphere_center, this->m_sphere_radius, vertices, indices);

	ModelPtr model = std::make_shared<Model>("Sphere",
		std::move(vertices),
		std::move(indices),
		std::vector<Material>{ *this->defaultMat },
		nullptr);

	return std::move(model);
}

Assets::ModelLibrary::ModelPtr  Assets::ModelLibrary::LoadCapsule()
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Capsule::Create(glm::vec3(0,0,0), this->m_capsule_radius, this->m_capsule_height, vertices, indices);

	ModelPtr model = std::make_shared<Model>("Capsule",
		std::move(vertices),
		std::move(indices),
		std::vector<Material>{ *this->defaultMat },
		nullptr);

	return std::move(model);
}

Assets::ModelLibrary::ModelPtr  Assets::ModelLibrary::LoadCylinder()
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Cylinder::Create(glm::vec3(0, 0, 0), this->m_cylinder_radius, this->m_capsule_height, vertices, indices);

	ModelPtr model = std::make_shared<Model>("Cylinder",
		std::move(vertices),
		std::move(indices),
		std::vector<Material>{ *this->defaultMat },
		nullptr);

	return std::move(model);
}

Assets::ModelLibrary::ModelPtr Assets::ModelLibrary::LoadCornellBox()
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	std::vector<Material> materials;

	CornellBox::Create(this->m_cornell_scale, vertices, indices, materials);

	ModelPtr model = std::make_shared<Model>("Cornell_Box",
		std::move(vertices),
		std::move(indices),
		std::move(materials),
		nullptr);

	return std::move(model);
}

Assets::ModelLibrary::ModelPtr Assets::ModelLibrary::LoadModel(const std::string& filePath)
{
	ModelPtr result = this->GetModel(filePath); //Look if it already exists

	if (!result)
	{
		std::cout << "- loading '" << filePath << "'... " << std::flush;

		const auto timer = std::chrono::high_resolution_clock::now();
		const std::string materialPath = std::filesystem::path(filePath).parent_path().string();

		Assimp::Importer objectImporter;
		const aiScene* scene = objectImporter.ReadFile(filePath, aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
			aiProcess_ImproveCacheLocality | aiProcess_LimitBoneWeights | aiProcess_SplitLargeMeshes | aiProcess_Triangulate |
			aiProcess_GenUVCoords | aiProcess_SortByPType | aiProcess_FindInvalidData | aiProcess_ValidateDataStructure | aiProcess_FlipUVs | 0);
		// read file and return an aiScene containing model attributes

		if (scene == nullptr)
		{
			Debug::Log("failed to load model '" + filePath + "':\n" + objectImporter.GetErrorString());
		}

		std::string name;
		int totalvertices = 0;

		for (int i = 0; i < scene->mNumMeshes; i++)
		{
			totalvertices += scene->mMeshes[i]->mNumVertices;
		}

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::unordered_map<Vertex, uint32_t> uniqueVertices;
		uniqueVertices.reserve(totalvertices);
		size_t faceId = 0;

		std::vector<Material> materials;
		aiColor4D diffuse;
		Material material{};
		for (int i = 0; i < scene->mNumMaterials; i++)
		{
			if (AI_SUCCESS != scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
			{

				material.Diffuse = glm::vec4(0.7f, 0.7f, 0.7f, 1.0);
				material.DiffuseTextureId = -1;

				std::cout << "No Texture in Mesh!" << std::endl;

			}
			else
			{
				material.Diffuse = glm::vec4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);

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
					material.DiffuseTextureId = -1;
				}
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
		glm::vec3 minPos(FLT_MAX);
		glm::vec3 maxPos(-FLT_MAX);
		for (const auto& vertex : vertices)
		{
			minPos = glm::min(minPos, vertex.Position);
			maxPos = glm::max(maxPos, vertex.Position);
		}

		glm::vec3 center = (minPos + maxPos) * 0.5f;

		// Shift all vertices so that the model is centered at the origin.
		for (auto& vertex : vertices)
		{
			vertex.Position -= center;
		}

		//// --- End centering ---
		const auto elapsed = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - timer).count();

		std::cout << "(" << totalvertices << " vertices, " << uniqueVertices.size() << " unique vertices, " << materials.size() << " materials) ";
		std::cout << elapsed << "s" << std::endl;

		auto model = std::make_shared<Model>(name, std::move(vertices), std::move(indices), std::move(materials), nullptr);
		model->filepath = filePath;

		this->m_meshMap.insert({ filePath, model });

		return model;
	}

	return result;
}

