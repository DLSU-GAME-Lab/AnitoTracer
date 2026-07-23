#include "ModelManager.hpp"

void ModelManager::Initialize(IRenderDevice* pDevice, IDeviceContext* mContext, const std::string& assetBasePath) {
    m_pDevice = pDevice;
    pContext = mContext;
    m_AssetBasePath = assetBasePath;

    LoadDefaultWhite();
}

/// <summary>
/// Create a 1x1 Default white tex
/// </summary>
void ModelManager::LoadDefaultWhite() {
    TextureDesc TexDesc;
    TexDesc.Name = "Default White Texture";
    TexDesc.Type = RESOURCE_DIM_TEX_2D;
    TexDesc.Width = 1;
    TexDesc.Height = 1;
    TexDesc.Format = TEX_FORMAT_RGBA8_UNORM_SRGB;
    TexDesc.BindFlags = BIND_SHADER_RESOURCE;
    TexDesc.Usage = USAGE_IMMUTABLE;

    Uint32 WhitePixel = 0xFFFFFFFF; // Pure white RGBA
    TextureSubResData SubresData[] = { {&WhitePixel, 4} };
    TextureData InitData(SubresData, 1);

    RefCntAutoPtr<ITexture> pDefaultTex;
    m_pDevice->CreateTexture(TexDesc, &InitData, &pDefaultTex);
    m_pDefaultTextureView = pDefaultTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
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
    loadInfo.GenerateMips = true;
    loadInfo.Format = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;

    std::filesystem::path modelFilePath(filepath);
    std::string fullPath = modelFilePath.is_absolute() ? filepath : (m_AssetBasePath + filepath);

    CreateTextureFromFile(fullPath.c_str(), loadInfo, m_pDevice, &pTexture);

    if (!pTexture) {
        std::cerr << "Failed to load texture: " << fullPath << std::endl;
        return nullptr;
    }

    RefCntAutoPtr<ITextureView> pSRV (pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    m_TextureCache[filepath] = pSRV;

    return pSRV;
}

ITextureView* ModelManager::LoadMaterialTexture(aiMaterial* material, aiTextureType type, const std::string& modelDir, bool& outHasProperty) {
    aiString texPath;
    if (material->GetTextureCount(type) > 0) {
        if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS && texPath.length > 0) {
            outHasProperty = true;
            std::string finalTexPath = modelDir + texPath.C_Str();
            return LoadTexture(finalTexPath);
        }
    }
    return nullptr;
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

    std::filesystem::path modelFilePath(filepath);
    std::string fullPath = modelFilePath.is_absolute() ? filepath : (m_AssetBasePath + filepath);

    Assimp::Importer importer;
    // Optimize for Vulkan/Modern APIs: Triangulate, Gen Normals, Flip UVs (Diligent uses Top-Left UVs)
    const aiScene* pScene = importer.ReadFile(fullPath,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals |
        aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

    if (!pScene || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !pScene->mRootNode) {
        std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return nullptr;
    }

    auto pModel = std::make_unique<Model>();
    std::vector<Vertex> vertices;
    std::vector<Uint32> indices;

    std::string modelDir = modelFilePath.parent_path().string();

    if (!modelDir.empty()) {
        modelDir += "/";
    }

    // 1. Process Materials & Textures
    pModel->Materials.resize(pScene->mNumMaterials);
    pModel->MaterialColors.resize(pScene->mNumMaterials, float4(1.0f, 1.0f, 1.0f, 1.0f));
    pModel->PBRMaterials.resize(pScene->mNumMaterials);
    bool modelHasAnyPBR = false;

    for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
        aiMaterial* material = pScene->mMaterials[i];
        PBRMaterial& pbrMat = pModel->PBRMaterials[i];
        bool currentMatHasPBR = false;

        aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
        if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            pbrMat.BaseColorFactor = float4(color.r, color.g, color.b, color.a);
        }

        // Metallic / Roughness Factors
        float metallic = 0.0f;
        if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
            pbrMat.MetallicFactor = metallic;
            currentMatHasPBR = true;
        }

        float roughness = 1.0f;
        if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
            pbrMat.RoughnessFactor = roughness;
            currentMatHasPBR = true;
        }

        // Load Textures using our class member helper function
        pbrMat.BaseColor = LoadMaterialTexture(material, aiTextureType_BASE_COLOR, modelDir, currentMatHasPBR);
        if (!pbrMat.BaseColor) {
            pbrMat.BaseColor = LoadMaterialTexture(material, aiTextureType_DIFFUSE, modelDir, currentMatHasPBR);
        }

        pbrMat.MetallicRoughness = LoadMaterialTexture(material, aiTextureType_METALNESS, modelDir, currentMatHasPBR);
        if (!pbrMat.MetallicRoughness) {
            pbrMat.MetallicRoughness = LoadMaterialTexture(material, aiTextureType_DIFFUSE_ROUGHNESS, modelDir, currentMatHasPBR);
        }

        pbrMat.Normal = LoadMaterialTexture(material, aiTextureType_NORMALS, modelDir, currentMatHasPBR);
        if (!pbrMat.Normal) {
            pbrMat.Normal = LoadMaterialTexture(material, aiTextureType_HEIGHT, modelDir, currentMatHasPBR);
        }

        pbrMat.AO = LoadMaterialTexture(material, aiTextureType_AMBIENT_OCCLUSION, modelDir, currentMatHasPBR);
        if (!pbrMat.AO) {
            pbrMat.AO = LoadMaterialTexture(material, aiTextureType_LIGHTMAP, modelDir, currentMatHasPBR);
        }

        pbrMat.Emissive = LoadMaterialTexture(material, aiTextureType_EMISSIVE, modelDir, currentMatHasPBR);

        // Fallback to default white if no base color texture was found
        if (!pbrMat.BaseColor) {
            pbrMat.BaseColor = m_pDefaultTextureView;
        }

        if (currentMatHasPBR) {
            modelHasAnyPBR = true;
        }
    }

    pModel->HasPBRProperties = modelHasAnyPBR;

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
                indices.push_back(face.mIndices[k] + submesh.BaseVertex);
            }
        }

        submesh.IndexCount = static_cast<Uint32>(indices.size()) - submesh.IndexOffset;
        pModel->SubMeshes.push_back(submesh);
    }

    // 3. Create Diligent Hardware Buffers
    BufferDesc VertBuffDesc;
    VertBuffDesc.Name = "Model Vertex Buffer";
    VertBuffDesc.Usage = USAGE_IMMUTABLE;
    // ADDED: BIND_RAY_TRACING is required for buffers used in BLAS building
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER | BIND_RAY_TRACING;
    VertBuffDesc.Size = vertices.size() * sizeof(Vertex);

    BufferData VBData;
    VBData.pData = vertices.data();
    VBData.DataSize = VertBuffDesc.Size;
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &pModel->pVertexBuffer);

    BufferDesc IndBuffDesc;
    IndBuffDesc.Name = "Model Index Buffer";
    IndBuffDesc.Usage = USAGE_IMMUTABLE;
    // ADDED: BIND_RAY_TRACING is required for buffers used in BLAS building
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER | BIND_RAY_TRACING;
    IndBuffDesc.Size = indices.size() * sizeof(Uint32);

    BufferData IBData;
    IBData.pData = indices.data();
    IBData.DataSize = IndBuffDesc.Size;
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &pModel->pIndexBuffer);

    // 4. Describe Acceleration Structure
    BLASTriangleDesc TriangleDesc;
    TriangleDesc.GeometryName = "ModelGeometry";
    TriangleDesc.MaxVertexCount = static_cast<Uint32>(vertices.size());
    TriangleDesc.VertexValueType = VT_FLOAT32;
    TriangleDesc.VertexComponentCount = 3;
    TriangleDesc.MaxPrimitiveCount = static_cast<Uint32>(indices.size()) / 3;
    TriangleDesc.IndexType = VT_UINT32;

    BottomLevelASDesc ASDesc;
    ASDesc.Name = "Model BLAS";
    ASDesc.Flags = RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;
    ASDesc.pTriangles = &TriangleDesc;
    ASDesc.TriangleCount = 1;

    m_pDevice->CreateBLAS(ASDesc, &pModel->pBLAS);

    // 5. Query Scratch Size & Allocate Scratch Buffer
    ScratchBufferSizes ScratchSizes = pModel->pBLAS->GetScratchBufferSizes();

    BufferDesc ScratchBuffDesc;
    ScratchBuffDesc.Name = "BLAS Build Scratch Buffer";
    ScratchBuffDesc.Size = ScratchSizes.Build;
    ScratchBuffDesc.Usage = USAGE_DEFAULT;
    ScratchBuffDesc.BindFlags = BIND_RAY_TRACING;

    RefCntAutoPtr<IBuffer> pScratchBuffer;
    m_pDevice->CreateBuffer(ScratchBuffDesc, nullptr, &pScratchBuffer);

    // 6. Build BLAS on GPU
    BLASBuildTriangleData TriData;
    TriData.GeometryName = "ModelGeometry";
    TriData.pVertexBuffer = pModel->pVertexBuffer;
    TriData.VertexStride = sizeof(Vertex);
    TriData.VertexOffset = 0;
    TriData.VertexCount = static_cast<Uint32>(vertices.size());
    TriData.VertexValueType = VT_FLOAT32;
    TriData.VertexComponentCount = 3;
    TriData.pIndexBuffer = pModel->pIndexBuffer;
    TriData.IndexType = VT_UINT32;
    TriData.IndexOffset = 0;
    TriData.PrimitiveCount = static_cast<Uint32>(indices.size()) / 3;

    BuildBLASAttribs BuildAttribs;
    BuildAttribs.pBLAS = pModel->pBLAS;
    BuildAttribs.pTriangleData = &TriData;
    BuildAttribs.TriangleDataCount = 1;
    BuildAttribs.pScratchBuffer = pScratchBuffer;

    // Transition the buffers so Vulkan can safely read/write during the BLAS build
    BuildAttribs.BLASTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.GeometryTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.ScratchBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    pContext->BuildBLAS(BuildAttribs);

    // Store in cache and return
    Model* rawPtr = pModel.get();
    m_ModelCache[filepath] = std::move(pModel);

    return rawPtr;
}

void ModelManager::ClearCache() {
    m_ModelCache.clear();
    m_TextureCache.clear();
}