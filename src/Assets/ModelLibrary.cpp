#include "ModelLibrary.hpp"

/* Default Models */
#include "Box.hpp"
#include "Plane.hpp"
#include "Capsule.hpp"
#include "Cylinder.hpp"
#include "Sphere.hpp"
#include "CornellBox.hpp"

#include "Utilities/FileUtils.h"
#include "From-GDGRAP2/MaterialLibrary.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <Assimp/postprocess.h>

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

Assets::ModelLibrary* Assets::ModelLibrary::sharedInstance = nullptr;

Assets::ModelLibrary::ModelLibrary()
{
	this->defaultMat = MaterialLibrary::getInstance()->getMaterial(L"White");
	LoadInitialModels();
}

Assets::ModelLoadResult Assets::ModelLibrary::GetModel(const String& meshName)
{
	ModelLoadResult result;

	auto it = this->m_meshMap.find(meshName);
	if (it != this->m_meshMap.end())
	{
        // Return copies
        for (size_t i = 0; i < it->second.modelsData.size(); i++)
        {
            auto copy = std::make_shared<Model>(*it->second.modelsData[i]);
            result.modelsData.push_back(copy);
            result.originalPositions.push_back(it->second.originalPositions[i]);
        }
	}
	else
	{
		Debug::Log("Model: '" + meshName + "' not found in ModelLibrary.");
	}

	return result;
}

void Assets::ModelLibrary::LoadInitialModels()
{
	/* Primitives */
    this->m_meshMap.insert({ "CUBE", { {this->LoadBox()}, {glm::vec3(0.0f)} } });
    this->m_meshMap.insert({ "PLANE", { {this->LoadPlane()}, {glm::vec3(0.0f)} } });
    this->m_meshMap.insert({ "SPHERE", { {this->LoadSphere()}, {glm::vec3(0.0f)} } });
    this->m_meshMap.insert({ "CYLINDER", { {this->LoadCylinder()}, {glm::vec3(0.0f)} } });
    this->m_meshMap.insert({ "CAPSULE", { {this->LoadCapsule()}, {glm::vec3(0.0f)} } });
    this->m_meshMap.insert({ "CORNELL_BOX", { {this->LoadCornellBox()}, {glm::vec3(0.0f)} } });
}

int Assets::ModelLibrary::GetInstanceId()
{
	return this->instancesIdCount++;
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
		nullptr
    );

	return std::move(model);
}

Assets::ModelLibrary::ModelPtr  Assets::ModelLibrary::LoadPlane()
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Plane::Create(this->m_plane_p0, this->m_plane_p1, vertices, indices);

	ModelPtr model = std::make_shared<Model>("Plane",
		std::move(vertices),
		std::move(indices),
		std::vector<Material>{ *this->defaultMat },
		nullptr
    );

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
		nullptr
    );

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
		nullptr
    );

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
		nullptr
    );

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
		nullptr
    );

	return std::move(model);
}

Assets::ModelLibrary* Assets::ModelLibrary::getInstance()
{
	return sharedInstance;
}

void Assets::ModelLibrary::initialize()
{
	sharedInstance = new ModelLibrary();
}

void Assets::ModelLibrary::destroy()
{
	delete sharedInstance;
}

Assets::ModelLoadResult Assets::ModelLibrary::LoadModel(const std::string& filePath)
{
	Assets::ModelLoadResult result = GetModel(filePath);
    if (!result.modelsData.empty()) return result;

    std::cout << "- loading '" << filePath << "'... " << std::flush;

    const auto timer = std::chrono::high_resolution_clock::now();
    const std::string materialPath = std::filesystem::path(filePath).parent_path().string();

    Assimp::Importer objectImporter;
    const aiScene* scene = objectImporter.ReadFile(filePath,
        aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices |
        aiProcess_ImproveCacheLocality | aiProcess_LimitBoneWeights | aiProcess_SplitLargeMeshes |
        aiProcess_Triangulate | aiProcess_GenUVCoords | aiProcess_SortByPType |
        aiProcess_FindInvalidData | aiProcess_ValidateDataStructure | aiProcess_FlipUVs);

    if (scene == nullptr)
    {
        Debug::Log("failed to load model '" + filePath + "':\n" + objectImporter.GetErrorString());
        return result;
    }

    // Load all materials from the scene
    std::vector<Material> allMaterials;
    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        Material material{};
        aiColor4D diffuse;

        if (AI_SUCCESS != scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
        {
            material.Diffuse = glm::vec4(0.7f, 0.7f, 0.7f, 1.0);
            material.DiffuseTextureId = -1;
            std::cout << "No Texture in Material " << i << std::endl;
        }
        else
        {
            material.Diffuse = glm::vec4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);

            int texcount = scene->mMaterials[i]->GetTextureCount(aiTextureType_DIFFUSE);
            if (texcount > 0)
            {
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
        allMaterials.push_back(material);
    }

    // Process each mesh separately
    size_t totalVertices = 0;
    size_t totalUniqueVertices = 0;
	Assets::ModelLoadResult loadResults;

    for (unsigned int m = 0; m < scene->mNumMeshes; m++)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<Vertex, uint32_t> uniqueVertices; // Per-mesh unique vertices
        const aiMesh* mesh = scene->mMeshes[m];

        // Process all faces in this mesh
        for (unsigned int f = 0; f < mesh->mNumFaces; f++)
        {
            for (unsigned int i = 0; i < mesh->mFaces[f].mNumIndices; i++)
            {
                unsigned int v = mesh->mFaces[f].mIndices[i];
                Vertex vertex = {};

                vertex.Position = {
                    mesh->mVertices[v].x,
                    mesh->mVertices[v].y,
                    mesh->mVertices[v].z
                };

                if (mesh->HasNormals())
                {
                    vertex.Normal = {
                        mesh->mNormals[v].x,
                        mesh->mNormals[v].y,
                        mesh->mNormals[v].z
                    };
                }
                else
                {
                    glm::vec3 normalized = glm::normalize(vertex.Position);
                    vertex.Normal = normalized;
                }

                if (mesh->HasTextureCoords(0))
                {
                    vertex.TexCoord = {
                        mesh->mTextureCoords[0][v].x,
                        mesh->mTextureCoords[0][v].y
                    };
                }

                vertex.MaterialIndex = 0;

                // Check if this vertex already exists
                if (uniqueVertices.find(vertex) == uniqueVertices.end())
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
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

		loadResults.originalPositions.push_back(center);

		// Shift all vertices so that the model is centered at the origin.
		for (auto& vertex : vertices)
		{
			vertex.Position -= center;
		}
		// --- End centering ---

        totalVertices += mesh->mNumVertices;
        totalUniqueVertices += vertices.size();

        // Get the material for this mesh
        std::vector<Material> meshMaterial;
        if (mesh->mMaterialIndex < allMaterials.size())
        {
            meshMaterial.push_back(allMaterials[mesh->mMaterialIndex]);
        }
        else
        {
            // Fallback material
            Material defaultMat{};
            defaultMat.Diffuse = glm::vec4(0.7f, 0.7f, 0.7f, 1.0);
            defaultMat.DiffuseTextureId = -1;
            meshMaterial.push_back(defaultMat);
        }

        // Create model for this mesh with its own material
        auto model = std::make_shared<Model>(
            "",
            std::move(vertices),
            std::move(indices),
            std::move(meshMaterial),  // Each mesh gets its own material copy
            nullptr
        );

        model->filepath = filePath;
        loadResults.modelsData.push_back(model);
    }

    const auto elapsed = std::chrono::duration<float, std::chrono::seconds::period>(
        std::chrono::high_resolution_clock::now() - timer).count();

    std::cout << "(" << totalVertices << " vertices, "
        << totalUniqueVertices << " unique vertices, "
        << loadResults.modelsData.size() << " meshes, "
        << allMaterials.size() << " materials) "
        << elapsed << "s" << std::endl;

    this->m_meshMap.insert({ filePath, loadResults });

    return loadResults;
}

	

