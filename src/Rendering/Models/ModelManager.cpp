#include "ModelManager.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>

void ModelManager::Initialize(IRenderDevice* pDevice, IDeviceContext* mContext, bool isRayTracingEnabled) {
    m_pDevice = pDevice;
    pContext = mContext;
    m_isRayTracingEnabled = isRayTracingEnabled;

    LoadDefaultWhite();
}

/// <summary>
/// Create a 1x1 Default white tex
/// </summary>
void ModelManager::LoadDefaultWhite() {
    TextureDesc TexDesc{};
    TexDesc.Name = "Default White Texture";
    TexDesc.Type = RESOURCE_DIM_TEX_2D;
    TexDesc.Width = 1;
    TexDesc.Height = 1;
    TexDesc.Format = TEX_FORMAT_RGBA8_UNORM_SRGB;
    TexDesc.BindFlags = BIND_SHADER_RESOURCE;
    TexDesc.Usage = USAGE_IMMUTABLE;

    Uint32 WhitePixel = 0xFFFFFFFF; // Pure white RGBA
    TextureSubResData SubresData[] = { { &WhitePixel, 4 } };
    TextureData InitData(SubresData, 1);

    RefCntAutoPtr<ITexture> pDefaultTex;
    m_pDevice->CreateTexture(TexDesc, &InitData, &pDefaultTex);
    m_pDefaultTextureView = pDefaultTex->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
}

ITextureView* ModelManager::LoadTexture(const std::string& filepath, bool isSRGB) {
    // Check cache first (cache key includes sRGB-ness to avoid returning a
    // texture loaded with the wrong gamma interpretation)
    std::string cacheKey = filepath + (isSRGB ? "|srgb" : "|linear");
    auto it = m_TextureCache.find(cacheKey);
    if (it != m_TextureCache.end()) {
        return it->second;
    }

    // Load texture using Diligent's utility
    RefCntAutoPtr<ITexture> pTexture;
    TextureLoadInfo loadInfo{};
    loadInfo.IsSRGB = isSRGB; // Only true for color data (BaseColor/Emissive).
    // Normal/Metallic-Roughness/AO maps store linear data and must NOT be gamma decoded.
    loadInfo.GenerateMips = true;
    loadInfo.Format = isSRGB ? Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB : Diligent::TEX_FORMAT_RGBA8_UNORM;

    std::filesystem::path modelFilePath(filepath);
    std::string fullPath = modelFilePath.string();

    CreateTextureFromFile(fullPath.c_str(), loadInfo, m_pDevice, &pTexture);

    if (!pTexture) {
        std::cerr << "Failed to load texture: " << fullPath << std::endl;
        return nullptr;
    }

    RefCntAutoPtr<ITextureView> pSRV(pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

    // Optimized cache insertion using emplace and moving the key string
    m_TextureCache.emplace(std::move(cacheKey), pSRV);

    return pSRV;
}

ITextureView* ModelManager::LoadMaterialTexture(aiMaterial* material, aiTextureType type, const std::string& modelDir, bool& outHasProperty, bool isSRGB) {
    aiString texPath;
    if (material->GetTextureCount(type) > 0) {
        if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS && texPath.length > 0) {
            outHasProperty = true;
            std::string finalTexPath = modelDir + texPath.C_Str();
            return LoadTexture(finalTexPath, isSRGB);
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
    std::string fullPath = modelFilePath.string();

    Assimp::Importer importer;
    // Optimize for Vulkan/Modern APIs: Triangulate, Gen Normals, Flip UVs (Diligent uses Top-Left UVs)
    const aiScene* pScene = importer.ReadFile(fullPath,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
        aiProcess_MakeLeftHanded | aiProcess_FlipUVs | aiProcess_FlipWindingOrder |
        aiProcess_JoinIdenticalVertices);

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

    // --- Pipeline Stages ---
    ProcessMaterials(pScene, pModel.get(), modelDir);
    ProcessMeshes(pScene, pModel.get(), vertices, indices);
    CreateHardwareBuffers(pModel.get(), vertices, indices);

    // CPU-side data for physics mesh colliders
    pModel->CollisionData = std::make_shared<CollisionMeshData>();
    pModel->CollisionData->vertices.reserve(vertices.size());
    for (const Vertex& v : vertices) {
        pModel->CollisionData->vertices.emplace_back(v.pos.x, v.pos.y, v.pos.z);
    }
    pModel->CollisionData->indices.assign(indices.begin(), indices.end());

    // Only build if raytracing is enabled
    if (m_isRayTracingEnabled) {
        BuildBLAS(pModel.get());
    }

    Model* rawPtr = pModel.get();

    // Optimized cache insertion
    m_ModelCache.emplace(filepath, std::move(pModel));

    return rawPtr;
}

void ModelManager::ProcessMaterials(const aiScene* pScene, Model* pModel, const std::string& modelDir) {
    pModel->Materials.resize(pScene->mNumMaterials);
    pModel->MaterialColors.resize(pScene->mNumMaterials, float4(1.0f, 1.0f, 1.0f, 1.0f));
    pModel->PBRMaterials.resize(pScene->mNumMaterials);

    bool modelHasAnyPBR = false;
    bool modelHasTransparency = false;

    for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
        aiMaterial* material = pScene->mMaterials[i];
        PBRMaterial& pbrMat = pModel->PBRMaterials[i];
        bool currentMatHasPBR = false;

        aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
        if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            pbrMat.BaseColorFactor = float4(color.r, color.g, color.b, color.a);
            if (color.a < 1.0f) pbrMat.IsTransparent = true;
        }

        float opacity = 1.0f;
        if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
            if (opacity < 1.0f) pbrMat.IsTransparent = true;
        }

        if (pbrMat.IsTransparent) {
            modelHasTransparency = true;
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

        // Texture extraction with standardized fallbacks
        pbrMat.BaseColor = LoadMaterialTexture(material, aiTextureType_BASE_COLOR, modelDir, currentMatHasPBR, true);
        if (!pbrMat.BaseColor) pbrMat.BaseColor = LoadMaterialTexture(material, aiTextureType_DIFFUSE, modelDir, currentMatHasPBR, true);

        pbrMat.MetallicRoughness = LoadMaterialTexture(material, aiTextureType_METALNESS, modelDir, currentMatHasPBR, false);
        if (!pbrMat.MetallicRoughness) pbrMat.MetallicRoughness = LoadMaterialTexture(material, aiTextureType_DIFFUSE_ROUGHNESS, modelDir, currentMatHasPBR, false);

        pbrMat.Normal = LoadMaterialTexture(material, aiTextureType_NORMALS, modelDir, currentMatHasPBR, false);
        if (!pbrMat.Normal) pbrMat.Normal = LoadMaterialTexture(material, aiTextureType_HEIGHT, modelDir, currentMatHasPBR, false);

        pbrMat.AO = LoadMaterialTexture(material, aiTextureType_AMBIENT_OCCLUSION, modelDir, currentMatHasPBR, false);
        if (!pbrMat.AO) pbrMat.AO = LoadMaterialTexture(material, aiTextureType_LIGHTMAP, modelDir, currentMatHasPBR, false);

        pbrMat.Emissive = LoadMaterialTexture(material, aiTextureType_EMISSIVE, modelDir, currentMatHasPBR, true);

        // Enforce fallback to default white if no base color texture was found
        if (!pbrMat.BaseColor) {
            pbrMat.BaseColor = m_pDefaultTextureView;
        }

        if (currentMatHasPBR) {
            modelHasAnyPBR = true;
        }
    }

    pModel->HasPBRProperties = modelHasAnyPBR;
    pModel->HasTransparency = modelHasTransparency;
}

void ModelManager::ProcessMeshes(const aiScene* pScene, Model* pModel, std::vector<Vertex>& outVertices, std::vector<Uint32>& outIndices) {
    for (unsigned int i = 0; i < pScene->mNumMeshes; i++) {
        aiMesh* mesh = pScene->mMeshes[i];
        SubMesh submesh{};
        submesh.BaseVertex = static_cast<Uint32>(outVertices.size());
        submesh.IndexOffset = static_cast<Uint32>(outIndices.size());
        submesh.MaterialIndex = mesh->mMaterialIndex;

        // Process Vertices
        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            Vertex v{};
            v.pos = float3(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z);

            // Dynamically calculate AABB limits
            pModel->AABBMin.x = std::min(pModel->AABBMin.x, v.pos.x);
            pModel->AABBMin.y = std::min(pModel->AABBMin.y, v.pos.y);
            pModel->AABBMin.z = std::min(pModel->AABBMin.z, v.pos.z);

            pModel->AABBMax.x = std::max(pModel->AABBMax.x, v.pos.x);
            pModel->AABBMax.y = std::max(pModel->AABBMax.y, v.pos.y);
            pModel->AABBMax.z = std::max(pModel->AABBMax.z, v.pos.z);

            if (mesh->HasNormals()) {
                v.normal = float3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z);
            }
            if (mesh->HasTangentsAndBitangents()) {
                v.tangent = float3(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z);
                v.bitangent = float3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z);
            }
            else {
                v.tangent = float3(0.0f, 0.0f, 0.0f);
                v.bitangent = float3(0.0f, 0.0f, 0.0f);
            }

            if (mesh->mTextureCoords[0]) {
                v.uv = float2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y);
            }
            else {
                v.uv = float2(0.0f, 0.0f);
            }

            outVertices.push_back(v);
        }

        // Process Indices
        for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
            aiFace face = mesh->mFaces[j];
            for (unsigned int k = 0; k < face.mNumIndices; k++) {
                outIndices.push_back(face.mIndices[k] + submesh.BaseVertex);
            }
        }

        submesh.IndexCount = static_cast<Uint32>(outIndices.size()) - submesh.IndexOffset;
        pModel->SubMeshes.push_back(submesh);
    }
}

void ModelManager::CreateHardwareBuffers(Model* pModel, const std::vector<Vertex>& vertices, const std::vector<Uint32>& indices) {
    // Generate Staged Vertex Buffer
    BufferDesc VertBuffDesc{};
    VertBuffDesc.Name = "Model Vertex Buffer";
    VertBuffDesc.Usage = USAGE_IMMUTABLE;
    VertBuffDesc.BindFlags = BIND_VERTEX_BUFFER;
    if (m_isRayTracingEnabled) {
        VertBuffDesc.BindFlags |= BIND_RAY_TRACING;
    }
    VertBuffDesc.Size = vertices.size() * sizeof(Vertex);

    BufferData VBData{};
    VBData.pData = vertices.data();
    VBData.DataSize = VertBuffDesc.Size;
    m_pDevice->CreateBuffer(VertBuffDesc, &VBData, &pModel->pVertexBuffer);

    // Generate Staged Index Buffer
    BufferDesc IndBuffDesc{};
    IndBuffDesc.Name = "Model Index Buffer";
    IndBuffDesc.Usage = USAGE_IMMUTABLE;
    IndBuffDesc.BindFlags = BIND_INDEX_BUFFER;
    if (m_isRayTracingEnabled) {
        IndBuffDesc.BindFlags |= BIND_RAY_TRACING;
    }
    IndBuffDesc.Size = indices.size() * sizeof(Uint32);

    BufferData IBData{};
    IBData.pData = indices.data();
    IBData.DataSize = IndBuffDesc.Size;
    m_pDevice->CreateBuffer(IndBuffDesc, &IBData, &pModel->pIndexBuffer);
}

void ModelManager::BuildBLAS(Model* pModel) {
    // Early exit if ray tracing is not enabled or if objects are uninitialized
    if (!m_isRayTracingEnabled || !pModel || !m_pDevice || !pContext) {
        return;
    }

    // Ensure buffers exist and BLAS has not already been built
    if (!pModel->pVertexBuffer || !pModel->pIndexBuffer || pModel->pBLAS) {
        return;
    }

    // Dynamically query sizes directly from the hardware buffers
    Uint32 vertexCount = static_cast<Uint32>(pModel->pVertexBuffer->GetDesc().Size / sizeof(Vertex));
    Uint32 indexCount = static_cast<Uint32>(pModel->pIndexBuffer->GetDesc().Size / sizeof(Uint32));
    Uint32 primitiveCount = indexCount / 3;

    // 1. Describe Acceleration Structure (Zero-initialized for safety)
    BLASTriangleDesc TriangleDesc{};
    TriangleDesc.GeometryName = "ModelGeometry";
    TriangleDesc.MaxVertexCount = vertexCount;
    TriangleDesc.VertexValueType = VT_FLOAT32;
    TriangleDesc.VertexComponentCount = 3;
    TriangleDesc.MaxPrimitiveCount = primitiveCount;
    TriangleDesc.IndexType = VT_UINT32;

    BottomLevelASDesc ASDesc{};
    ASDesc.Name = "Model BLAS";
    ASDesc.Flags = RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;
    ASDesc.pTriangles = &TriangleDesc;
    ASDesc.TriangleCount = 1;

    m_pDevice->CreateBLAS(ASDesc, &pModel->pBLAS);

    if (!pModel->pBLAS)
        return;

    // 2. Query Scratch Size & Allocate Scratch Buffer
    ScratchBufferSizes ScratchSizes = pModel->pBLAS->GetScratchBufferSizes();

    BufferDesc ScratchBuffDesc{};
    ScratchBuffDesc.Name = "BLAS Build Scratch Buffer";
    ScratchBuffDesc.Size = ScratchSizes.Build;
    ScratchBuffDesc.Usage = USAGE_DEFAULT;
    ScratchBuffDesc.BindFlags = BIND_RAY_TRACING;

    RefCntAutoPtr<IBuffer> pScratchBuffer;
    m_pDevice->CreateBuffer(ScratchBuffDesc, nullptr, &pScratchBuffer);

    // 3. Build BLAS on GPU (Zero-initialized descriptor)
    BLASBuildTriangleData TriData{};
    TriData.GeometryName = "ModelGeometry";
    TriData.pVertexBuffer = pModel->pVertexBuffer;
    TriData.VertexStride = sizeof(Vertex);
    TriData.VertexOffset = 0;
    TriData.VertexCount = vertexCount;
    TriData.VertexValueType = VT_FLOAT32;
    TriData.VertexComponentCount = 3;
    TriData.pIndexBuffer = pModel->pIndexBuffer;
    TriData.IndexType = VT_UINT32;
    TriData.IndexOffset = 0;
    TriData.PrimitiveCount = primitiveCount;

    BuildBLASAttribs BuildAttribs{};
    BuildAttribs.pBLAS = pModel->pBLAS;
    BuildAttribs.pTriangleData = &TriData;
    BuildAttribs.TriangleDataCount = 1;
    BuildAttribs.pScratchBuffer = pScratchBuffer;

    // Transition the buffers so Vulkan can safely read/write during the BLAS build
    BuildAttribs.BLASTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.GeometryTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.ScratchBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    pContext->BuildBLAS(BuildAttribs);
}

void ModelManager::ClearCache() {
    m_ModelCache.clear();
    m_TextureCache.clear();
}