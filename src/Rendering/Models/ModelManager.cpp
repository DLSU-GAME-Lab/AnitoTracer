#include "ModelManager.hpp"
#include "ModelManager.hpp"

void ModelManager::Initialize(IRenderDevice* pDevice, IDeviceContext* mContext) {
    m_pDevice = pDevice;
    pContext = mContext;

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
    TextureLoadInfo loadInfo;
    loadInfo.IsSRGB = isSRGB; // Only true for color data (BaseColor/Emissive).
                              // Normal/Metallic-Roughness/AO maps store linear
                              // data and must NOT be gamma decoded.
    loadInfo.GenerateMips = true;
    loadInfo.Format = isSRGB ? Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB : Diligent::TEX_FORMAT_RGBA8_UNORM;

    std::filesystem::path modelFilePath(filepath);
    std::string fullPath = modelFilePath.string();

    CreateTextureFromFile(fullPath.c_str(), loadInfo, m_pDevice, &pTexture);

    if (!pTexture) {
        std::cerr << "Failed to load texture: " << fullPath << std::endl;
        return nullptr;
    }

    RefCntAutoPtr<ITextureView> pSRV (pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    m_TextureCache[cacheKey] = pSRV;

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

    // 1. Process Materials & Textures
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

            if (color.a < 1.0f) {
                pbrMat.IsTransparent = true;
            }
        }

        float opacity = 1.0f;
        if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
            if (opacity < 1.0f) {
                pbrMat.IsTransparent = true;
            }
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

        // Load Textures using our class member helper function.
        // Only BaseColor/Emissive are color data and need sRGB gamma decode.
        // Normal, MetallicRoughness, and AO maps store linear data - loading
        // them as sRGB distorts values (especially near the 0.5 "flat"
        // midpoint used by normal maps), which was causing incorrect
        // lighting/normals across flat surface regions.
        pbrMat.BaseColor = LoadMaterialTexture(material, aiTextureType_BASE_COLOR, modelDir, currentMatHasPBR, /*isSRGB=*/true);
        if (!pbrMat.BaseColor) {
            pbrMat.BaseColor = LoadMaterialTexture(material, aiTextureType_DIFFUSE, modelDir, currentMatHasPBR, /*isSRGB=*/true);
        }

        pbrMat.MetallicRoughness = LoadMaterialTexture(material, aiTextureType_METALNESS, modelDir, currentMatHasPBR, /*isSRGB=*/false);
        if (!pbrMat.MetallicRoughness) {
            pbrMat.MetallicRoughness = LoadMaterialTexture(material, aiTextureType_DIFFUSE_ROUGHNESS, modelDir, currentMatHasPBR, /*isSRGB=*/false);
        }

        pbrMat.Normal = LoadMaterialTexture(material, aiTextureType_NORMALS, modelDir, currentMatHasPBR, /*isSRGB=*/false);
        if (!pbrMat.Normal) {
            pbrMat.Normal = LoadMaterialTexture(material, aiTextureType_HEIGHT, modelDir, currentMatHasPBR, /*isSRGB=*/false);
        }

        pbrMat.AO = LoadMaterialTexture(material, aiTextureType_AMBIENT_OCCLUSION, modelDir, currentMatHasPBR, /*isSRGB=*/false);
        if (!pbrMat.AO) {
            pbrMat.AO = LoadMaterialTexture(material, aiTextureType_LIGHTMAP, modelDir, currentMatHasPBR, /*isSRGB=*/false);
        }

        pbrMat.Emissive = LoadMaterialTexture(material, aiTextureType_EMISSIVE, modelDir, currentMatHasPBR, /*isSRGB=*/true);

        // Fallback to default white if no base color texture was found
        if (!pbrMat.BaseColor) {
            pbrMat.BaseColor = m_pDefaultTextureView;
        }

        if (currentMatHasPBR) {
            modelHasAnyPBR = true;
        }
    }

    pModel->HasPBRProperties = modelHasAnyPBR;
	pModel->HasTransparency = modelHasTransparency;

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
                v.uv = float4(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y, 0.0f, 0.0f);
            }
            else {
                v.uv = float4(0.0f, 0.0f, 0.0f, 0.0f);
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

    pModel->CPUVertices = vertices;
    pModel->CPUIndices = indices;

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
    std::vector<std::string> GeometryNames(pModel->SubMeshes.size());
    std::vector<BLASTriangleDesc> TriangleDescs(pModel->SubMeshes.size());

    for (size_t i = 0; i < pModel->SubMeshes.size(); ++i) {
        auto& submesh = pModel->SubMeshes[i];

        GeometryNames[i] = "Submesh_" + std::to_string(i);
        TriangleDescs[i].GeometryName = GeometryNames[i].c_str(); // Now it's a safe pointer!

        TriangleDescs[i].MaxVertexCount = static_cast<Uint32>(vertices.size());
        TriangleDescs[i].VertexValueType = VT_FLOAT32;
        TriangleDescs[i].VertexComponentCount = 3;
        TriangleDescs[i].MaxPrimitiveCount = submesh.IndexCount / 3;
        TriangleDescs[i].IndexType = VT_UINT32;
    }

    BottomLevelASDesc ASDesc;
    ASDesc.Name = "Model BLAS";
    ASDesc.Flags = RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;
    ASDesc.pTriangles = TriangleDescs.data();
    ASDesc.TriangleCount = static_cast<Uint32>(TriangleDescs.size());

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
    std::vector<BLASBuildTriangleData> TriDatas(pModel->SubMeshes.size());
    for (size_t i = 0; i < pModel->SubMeshes.size(); ++i) {
        auto& submesh = pModel->SubMeshes[i];
        TriDatas[i].GeometryName = TriangleDescs[i].GeometryName;
        TriDatas[i].pVertexBuffer = pModel->pVertexBuffer;
        TriDatas[i].VertexStride = sizeof(Vertex);
        TriDatas[i].VertexOffset = 0;
        TriDatas[i].VertexCount = static_cast<Uint32>(vertices.size());
        TriDatas[i].VertexValueType = VT_FLOAT32;
        TriDatas[i].VertexComponentCount = 3;
        TriDatas[i].pIndexBuffer = pModel->pIndexBuffer;
        TriDatas[i].IndexType = VT_UINT32;

        // Offset in bytes for this specific submesh
        TriDatas[i].IndexOffset = submesh.IndexOffset * sizeof(Uint32);
        TriDatas[i].PrimitiveCount = submesh.IndexCount / 3;

        // Apply hardware acceleration flags here
        if (!pModel->PBRMaterials[submesh.MaterialIndex].IsTransparent) {
            TriDatas[i].Flags = RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        }
        else {
            TriDatas[i].Flags = RAYTRACING_GEOMETRY_FLAG_NONE;
        }
    }

    BuildBLASAttribs BuildAttribs;
    BuildAttribs.pBLAS = pModel->pBLAS;
    BuildAttribs.pTriangleData = TriDatas.data();
    BuildAttribs.TriangleDataCount = static_cast<Uint32>(TriDatas.size());
    BuildAttribs.pScratchBuffer = pScratchBuffer;

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