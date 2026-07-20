#include "ModelManager.hpp"

void ModelManager::Initialize(IRenderDevice* pDevice, const std::string& assetBasePath) {
    m_pDevice = pDevice;
    m_AssetBasePath = assetBasePath;
}

ITextureView* ModelManager::LoadTexture(const std::string& filepath) {
    // Check cache first
    auto it = m_TextureCache.find(filepath);
    if (it != m_TextureCache.end()) {
        return it->second;
    }

    // Load texture using Diligent's utility
    RefCntAutoPtr<ITexture> pTexture;
    TextureLoadInfo loadInfo;
    loadInfo.IsSRGB = true; // Typically true for diffuse textures

    std::string fullPath = m_AssetBasePath + filepath;
    CreateTextureFromFile(fullPath.c_str(), loadInfo, m_pDevice, &pTexture);

    if (!pTexture) {
        std::cerr << "Failed to load texture: " << fullPath << std::endl;
        return nullptr;
    }

    RefCntAutoPtr<ITextureView> pSRV (pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    m_TextureCache[filepath] = pSRV;

    return pSRV;
}

Model* ModelManager::LoadModel(const std::string& filepath) {
    if (!m_pDevice) {
        std::cerr << "ModelManager not initialized with RenderDevice!" << std::endl;
        return nullptr;
    }

    // Check cache
    auto it = m_ModelCache.find(filepath);
    if (it != m_ModelCache.end()) {
        return it->second.get();
    }

    Assimp::Importer importer;
    // Optimize for Vulkan/Modern APIs: Triangulate, Gen Normals, Flip UVs (Diligent uses Top-Left UVs)
    const aiScene* pScene = importer.ReadFile(m_AssetBasePath + filepath,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

    if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return nullptr;
    }

    auto pModel = std::make_unique<Model>();
    std::vector<Vertex> vertices;
    std::vector<Uint32> indices;

    // 1. Process Materials & Textures
    pModel->Materials.resize(pScene->mNumMaterials);
    for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
        aiMaterial* material = pScene->mMaterials[i];
        if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            aiString texPath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
            // Assuming texture path is relative to the asset folder
            pModel->Materials[i] = LoadTexture(texPath.C_Str());
        }
    }

    // 2. Process Meshes
    for (unsigned int i = 0; i < pScene->mNumMeshes; i++) {
        aiMesh* mesh = pScene->mMeshes[i];
        SubMesh submesh;
        submesh.BaseVertex = static_cast<Uint32>(vertices.size());
        submesh.IndexOffset = static_cast<Uint32>(indices.size());
        submesh.MaterialIndex = mesh->mMaterialIndex;

        // Vertices
        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            Vertex v;
            v.pos = float3(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z);
            if (mesh->HasNormals()) {
                v.normal = float3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z);
            }
            if (mesh->mTextureCoords[0]) {
                v.uv = float2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
            }
            else {
                v.uv = float2(0.0f, 0.0f);
            }
            vertices.push_back(v);
        }

        // Indices
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                indices.push_back(face.mIndices[k]);
            }
        }

        submesh.IndexCount = static_cast<Uint32>(indices.size()) - submesh.IndexOffset;
        pModel->SubMeshes.push_back(submesh);
    }

    // 3. Create Diligent Hardware Buffers
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name = "Model Vertex Buffer";
    VertBuffDesc.Usage = USAGE_IMMUTABLE; // Immutable is optimal for Vulkan
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    VertBuffDesc.Size = vertices.size() * sizeof(Vertex);

    BufferData VBData;
    VBData.pData = vertices.data();
    VBData.DataSize = VertBuffDesc.Size;
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &pModel->pVertexBuffer);

    BufferDesc IndBuffDesc;
    IndBuffDesc.Name = "Model Index Buffer";
    IndBuffDesc.Usage = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    IndBuffDesc.Size = indices.size() * sizeof(Uint32);

    BufferData IBData;
    IBData.pData = indices.data();
    IBData.DataSize = IndBuffDesc.Size;
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &pModel->pIndexBuffer);

    // Store in cache and return
    Model* rawPtr = pModel.get();
    m_ModelCache[filepath] = std::move(pModel);

    return rawPtr;
}

void ModelManager::ClearCache() {
    m_ModelCache.clear();
    m_TextureCache.clear();
}