#include "ModelManager.hpp"
#include <iostream>
#include <filesystem>
#include <algorithm>

void ModelManager::Initialize(IRenderDevice* pDevice, IDeviceContext* mContext, bool isRayTracingEnabled) {
    m_pDevice = pDevice;
    pContext = mContext;
    m_isRayTracingEnabled = isRayTracingEnabled;

    LoadDefaultWhite();
    m_ScenePool.Initialize(pDevice, isRayTracingEnabled);
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

namespace {
    // Only color data (base color, emissive) is gamma decoded. Normal, metallic-roughness and AO maps are linear:
    // reading them as sRGB would distort them, most visibly the "flat" middle of a normal map.
    TextureLoadInfo MakeTextureLoadInfo(bool isSRGB) {
        TextureLoadInfo loadInfo{};
        loadInfo.IsSRGB = isSRGB;
        loadInfo.GenerateMips = true;
        loadInfo.Format = isSRGB ? Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB : Diligent::TEX_FORMAT_RGBA8_UNORM;
        return loadInfo;
    }
}

ITextureView* ModelManager::LoadTexture(const std::string& filepath, bool isSRGB) {
    // Check cache first (cache key includes sRGB-ness to avoid returning a
    // texture loaded with the wrong gamma interpretation)
    std::string cacheKey = filepath + (isSRGB ? "|srgb" : "|linear");
    auto it = m_TextureCache.find(cacheKey);
    if (it != m_TextureCache.end()) {
        return it->second;
    }

    std::filesystem::path modelFilePath(filepath);
    std::string fullPath = modelFilePath.string();

    RefCntAutoPtr<ITextureLoader> pLoader;
    CreateTextureLoaderFromFile(fullPath.c_str(), IMAGE_FILE_FORMAT_UNKNOWN, MakeTextureLoadInfo(isSRGB), &pLoader);
    return FinishTexture(pLoader, std::move(cacheKey), fullPath);
}

ITextureView* ModelManager::LoadEmbeddedTexture(const aiTexture* pEmbedded, const std::string& name, bool isSRGB) {
    std::string cacheKey = name + (isSRGB ? "|srgb" : "|linear");
    auto it = m_TextureCache.find(cacheKey);
    if (it != m_TextureCache.end()) {
        return it->second;
    }

    if (!pEmbedded || !pEmbedded->pcData) {
        return nullptr;
    }

    // glTF and GLB keep their images as PNG or JPEG data: mHeight is 0 and mWidth is the size in bytes
    if (pEmbedded->mHeight != 0) {
        std::cerr << "Embedded texture is not a compressed image, skipped: " << name << std::endl;
        return nullptr;
    }

    RefCntAutoPtr<ITextureLoader> pLoader;
    CreateTextureLoaderFromMemory(static_cast<const void*>(pEmbedded->pcData), pEmbedded->mWidth, /*MakeCopy*/ true,
        MakeTextureLoadInfo(isSRGB), &pLoader);
    return FinishTexture(pLoader, std::move(cacheKey), name);
}

ITextureView* ModelManager::FinishTexture(ITextureLoader* pLoader, std::string cacheKey, const std::string& label) {
    RefCntAutoPtr<ITexture> pTexture;
    if (pLoader) {
        pLoader->CreateTexture(m_pDevice, &pTexture);
    }

    if (!pTexture) {
        std::cerr << "Failed to load texture: " << label << std::endl;
        return nullptr;
    }

    RefCntAutoPtr<ITextureView> pSRV(pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

    // Does the picture have gaps? A few stray see-through texels (an antialiased edge) don't count; a fence's or
    // a leaf card's surroundings do. Only the top mip is looked at; the loader forced RGBA8 above.
    {
        const TextureDesc& desc = pLoader->GetTextureDesc();
        const TextureSubResData& top = pLoader->GetSubresourceData(0, 0);
        if (top.pData && desc.Format != TEX_FORMAT_UNKNOWN) {
            const Uint8* rows = static_cast<const Uint8*>(top.pData);
            Uint64 clear = 0;
            for (Uint32 y = 0; y < desc.Height; ++y) {
                const Uint8* row = rows + static_cast<Uint64>(y) * top.Stride;
                for (Uint32 x = 0; x < desc.Width; ++x) {
                    if (row[x * 4 + 3] < 128) ++clear;
                }
            }
            if (clear * 500 > static_cast<Uint64>(desc.Width) * desc.Height) m_CutoutTextures.insert(pSRV.RawPtr());
        }
    }

    // Optimized cache insertion using emplace and moving the key string
    m_TextureCache.emplace(std::move(cacheKey), pSRV);

    return pSRV;
}

namespace {
    bool IsGltfPath(const std::string& path) {
        std::string extension = std::filesystem::path(path).extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return extension == ".gltf" || extension == ".glb";
    }
}

bool ModelManager::HasCutoutAlpha(ITextureView* pTexture) const {
    return pTexture && m_CutoutTextures.count(pTexture) != 0;
}

ITextureView* ModelManager::LoadCubemap(const std::string& filepath, bool isSRGB)
{
    // Check cache first, using a unique cubemap identifier to avoid collisions with 2D textures
    std::string cacheKey = filepath + (isSRGB ? "|cubemap_srgb" : "|cubemap_linear");
    auto it = m_TextureCache.find(cacheKey);
    if (it != m_TextureCache.end()) {
        return it->second;
    }

    std::filesystem::path modelFilePath(filepath);
    std::string fullPath = modelFilePath.string();

    // Ensure the file actually exists before sending it to Diligent[cite: 20]
    if (!std::filesystem::exists(modelFilePath)) {
        std::cerr << "[ModelManager] Master, the skybox file is missing! " << fullPath << std::endl;
        return nullptr;
    }

    RefCntAutoPtr<ITexture> pTexture;
    TextureLoadInfo loadInfo{};
    loadInfo.IsSRGB = isSRGB;

    // ✨ Maid's Fix: Do NOT force RGBA8! 
    // Setting it to UNKNOWN lets Diligent read the native compressed format directly from the DDS header.
    loadInfo.Format = Diligent::TEX_FORMAT_UNKNOWN;

    // DDS cubemaps should ideally have mipmaps baked in during export. 
    // We let Diligent generate them just in case they are missing!
    loadInfo.GenerateMips = true;

    // Diligent's utility parses the .dds header and automatically assigns RESOURCE_DIM_TEX_CUBE[cite: 20]
    CreateTextureFromFile(fullPath.c_str(), loadInfo, m_pDevice, &pTexture);

    if (!pTexture) {
        std::cerr << "[ModelManager] Failed to load cubemap (Check if it's a valid DDS!): " << fullPath << std::endl;
        return nullptr;
    }

    // ✨ Maid's Safety Check: Ensure the engine actually created a TextureCube![cite: 20]
    if (pTexture->GetDesc().Type != RESOURCE_DIM_TEX_CUBE) {
        std::cerr << "[ModelManager] Error: Texture is not a cubemap! Did you use a standard 2D image? " << fullPath << std::endl;
        return nullptr;
    }

    RefCntAutoPtr<ITextureView> pSRV(pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

    // Optimized cache insertion[cite: 20]
    m_TextureCache.emplace(std::move(cacheKey), pSRV);

    return pSRV;
}

ITextureView* ModelManager::LoadMaterialTexture(const aiScene* pScene, const std::string& modelPath, aiMaterial* material, aiTextureType type,
    const std::string& modelDir, bool& outHasProperty, bool isSRGB) {
    aiString texPath;
    if (material->GetTextureCount(type) > 0) {
        if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS && texPath.length > 0) {
            outHasProperty = true;

            // An image inside the model file (GLB, or a glTF with the image as a data URI) is named "*0", "*1", ...
            if (const aiTexture* pEmbedded = pScene->GetEmbeddedTexture(texPath.C_Str())) {
                return LoadEmbeddedTexture(pEmbedded, modelPath + "|" + texPath.C_Str(), isSRGB);
            }

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
    unsigned int importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace |
        aiProcess_MakeLeftHanded | aiProcess_FlipWindingOrder | aiProcess_JoinIdenticalVertices;
    // (assimp turns glTF's UVs to start at the bottom left like the other formats' before this flips them)
    importFlags |= aiProcess_FlipUVs;
    if (IsGltfPath(fullPath)) {
        // glTF puts each mesh's place in the scene on its nodes (many models are stored turned or scaled), and a model here
        // is one flat list of meshes, so the nodes' transforms are baked into the vertices
        importFlags |= aiProcess_PreTransformVertices;
    }
    const aiScene* pScene = importer.ReadFile(fullPath, importFlags);

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
    ProcessMaterials(pScene, pModel.get(), modelDir, fullPath);
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

void ModelManager::ProcessMaterials(const aiScene* pScene, Model* pModel, const std::string& modelDir, const std::string& modelPath) {
    const bool isGltf = IsGltfPath(modelPath);
    pModel->Materials.resize(pScene->mNumMaterials);
    pModel->MaterialColors.resize(pScene->mNumMaterials, float4(1.0f, 1.0f, 1.0f, 1.0f));
    pModel->PBRMaterials.resize(pScene->mNumMaterials);

    bool modelHasAnyPBR = false;
    bool modelHasTransparency = false;
    bool modelHasAlphaGeometry = false;

    for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
        aiMaterial* material = pScene->mMaterials[i];
        PBRMaterial& pbrMat = pModel->PBRMaterials[i];
        bool currentMatHasPBR = false;

        aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
        if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS ||
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            pbrMat.BaseColorFactor = float4(color.r, color.g, color.b, color.a);
        }

        // OBJ and FBX give the opacity as a number of its own (d, Tr)
        float opacity = 1.0f;
        const bool hasOpacity = material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS;

        float refractiveIndex = 1.5f;
        if (material->Get(AI_MATKEY_REFRACTI, refractiveIndex) == AI_SUCCESS) {
            pbrMat.IOR = std::clamp(refractiveIndex, 1.0f, 3.0f);
        }

        // Metallic / Roughness Factors
        float metallic = 0.0f;
        if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
            pbrMat.MetallicFactor = metallic;
            currentMatHasPBR = true;
        }

        float roughness = 1.0f;
        const bool hasRoughness = material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS;
        if (hasRoughness) {
            pbrMat.RoughnessFactor = roughness;
            currentMatHasPBR = true;
        }

        // Texture extraction with standardized fallbacks
        pbrMat.BaseColor = LoadMaterialTexture(pScene, modelPath, material, aiTextureType_BASE_COLOR, modelDir, currentMatHasPBR, true);
        if (!pbrMat.BaseColor) pbrMat.BaseColor = LoadMaterialTexture(pScene, modelPath, material, aiTextureType_DIFFUSE, modelDir, currentMatHasPBR, true);

        // Alpha mode. glTF names it. OBJ and FBX don't, so it is worked out from the opacity and the base color map:
        // a see-through factor means glass, a map with gaps means a cutout, and anything else is opaque.
        aiString alphaModeName;
        if (material->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaModeName) == AI_SUCCESS) {
            const std::string mode = alphaModeName.C_Str();
            if (mode == "MASK") {
                pbrMat.AlphaMode = MaterialAlphaMode::Mask;
                float cutoff = 0.5f;
                material->Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff);
                pbrMat.AlphaCutoff = std::clamp(cutoff, 0.01f, 1.0f);
            }
            else if (mode == "BLEND") {
                pbrMat.AlphaMode = MaterialAlphaMode::Blend;
            }
        }
        else if (!isGltf && (pbrMat.BaseColorFactor.a < 1.0f || (hasOpacity && opacity < 1.0f))) {
            pbrMat.AlphaMode = MaterialAlphaMode::Blend;
            // The opacity is a number of its own, apart from the color's alpha (which these formats leave at 1)
            if (hasOpacity) pbrMat.BaseColorFactor.a *= opacity;
        }
        else if (!isGltf && HasCutoutAlpha(pbrMat.BaseColor)) {
            pbrMat.AlphaMode = MaterialAlphaMode::Mask;
            pbrMat.AlphaCutoff = 0.5f;
        }
        pbrMat.IsTransparent = pbrMat.AlphaMode == MaterialAlphaMode::Blend;
        if (pbrMat.AlphaMode != MaterialAlphaMode::Opaque) modelHasAlphaGeometry = true;
        if (pbrMat.IsTransparent) {
            modelHasTransparency = true;
            // Glass is smooth; formats without a roughness would otherwise leave it at the rough default
            if (!hasRoughness) pbrMat.RoughnessFactor = 0.05f;
        }

        // glTF packs roughness (green) and metallic (blue) into one image, which has a slot of its own
        pbrMat.MetallicRoughness = LoadMaterialTexture(pScene, modelPath, material, aiTextureType_GLTF_METALLIC_ROUGHNESS, modelDir, currentMatHasPBR, false);
        if (!pbrMat.MetallicRoughness) pbrMat.MetallicRoughness = LoadMaterialTexture(pScene, modelPath, material, aiTextureType_METALNESS, modelDir, currentMatHasPBR, false);
        if (!pbrMat.MetallicRoughness) pbrMat.MetallicRoughness = LoadMaterialTexture(pScene, modelPath, material, aiTextureType_DIFFUSE_ROUGHNESS, modelDir, currentMatHasPBR, false);

        pbrMat.Normal = LoadMaterialTexture(pScene, modelPath, material, aiTextureType_NORMALS, modelDir, currentMatHasPBR, false);
        if (!pbrMat.Normal) pbrMat.Normal = LoadMaterialTexture(pScene, modelPath, material, aiTextureType_HEIGHT, modelDir, currentMatHasPBR, false);

        pbrMat.AO = LoadMaterialTexture(pScene, modelPath, material, aiTextureType_AMBIENT_OCCLUSION, modelDir, currentMatHasPBR, false);
        if (!pbrMat.AO) pbrMat.AO = LoadMaterialTexture(pScene, modelPath, material, aiTextureType_LIGHTMAP, modelDir, currentMatHasPBR, false);

        pbrMat.Emissive = LoadMaterialTexture(pScene, modelPath, material, aiTextureType_EMISSIVE, modelDir, currentMatHasPBR, true);

        // Emissive color (glTF emissiveFactor, OBJ Ke) and strength (KHR_materials_emissive_strength)
        aiColor3D emissiveColor(0.0f, 0.0f, 0.0f);
        bool hasEmissiveColor = material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS;
        pbrMat.EmissiveFactor = float3(emissiveColor.r, emissiveColor.g, emissiveColor.b);

        // A map with no (or black) color would never glow; exporters often leave Ke at 0 next to map_Ke, so let the map through
        if (pbrMat.Emissive && (!hasEmissiveColor || emissiveColor.IsBlack())) {
            pbrMat.EmissiveFactor = float3(1.0f, 1.0f, 1.0f);
        }

        float emissiveStrength = 1.0f;
        if (material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveStrength) == AI_SUCCESS) {
            pbrMat.EmissiveStrength = emissiveStrength;
        }

        // The maps a reflection shows: ray hits look these up in the scene texture array.
        // (Before the default white fallback below, which needs no slot: a hit without a map uses the color factor alone.)
        m_ScenePool.RegisterTexture(pbrMat.BaseColor);
        m_ScenePool.RegisterTexture(pbrMat.MetallicRoughness);
        m_ScenePool.RegisterTexture(pbrMat.Emissive);

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
    pModel->HasAlphaGeometry = modelHasAlphaGeometry;
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

    // A copy for ray hits, which can land on any model and so can't use per-model buffers
    m_ScenePool.AddModel(*pModel, vertices, indices);
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

    // One geometry per submesh, so a ray hit can tell which submesh (and so which material) it landed on.
    // All of them share the model's vertex and index buffers; each covers its own range of the index buffer.
    pModel->BLASSubmeshes.clear();
    for (Uint32 i = 0; i < pModel->SubMeshes.size(); ++i) {
        // A geometry must have at least one triangle
        if (pModel->SubMeshes[i].IndexCount >= 3) pModel->BLASSubmeshes.push_back(i);
    }
    if (pModel->BLASSubmeshes.empty())
        return;

    const Uint32 geometryCount = static_cast<Uint32>(pModel->BLASSubmeshes.size());

    // The descriptions only point at their names, so the strings must outlive CreateBLAS and BuildBLAS
    std::vector<std::string> geometryNames(geometryCount);
    for (Uint32 g = 0; g < geometryCount; ++g) {
        geometryNames[g] = "Submesh_" + std::to_string(pModel->BLASSubmeshes[g]);
    }

    // 1. Describe Acceleration Structure
    std::vector<BLASTriangleDesc> TriangleDescs(geometryCount);
    for (Uint32 g = 0; g < geometryCount; ++g) {
        const SubMesh& submesh = pModel->SubMeshes[pModel->BLASSubmeshes[g]];

        BLASTriangleDesc& TriangleDesc = TriangleDescs[g];
        TriangleDesc.GeometryName = geometryNames[g].c_str();
        TriangleDesc.MaxVertexCount = vertexCount;
        TriangleDesc.VertexValueType = VT_FLOAT32;
        TriangleDesc.VertexComponentCount = 3;
        TriangleDesc.MaxPrimitiveCount = submesh.IndexCount / 3;
        TriangleDesc.IndexType = VT_UINT32;
    }

    BottomLevelASDesc ASDesc{};
    ASDesc.Name = "Model BLAS";
    ASDesc.Flags = RAYTRACING_BUILD_AS_PREFER_FAST_TRACE;
    ASDesc.pTriangles = TriangleDescs.data();
    ASDesc.TriangleCount = geometryCount;

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

    // 3. Build BLAS on GPU. The order here is the geometry index a ray hit reports, so it must match BLASSubmeshes.
    std::vector<BLASBuildTriangleData> TriData(geometryCount);
    for (Uint32 g = 0; g < geometryCount; ++g) {
        const SubMesh& submesh = pModel->SubMeshes[pModel->BLASSubmeshes[g]];

        BLASBuildTriangleData& Tri = TriData[g];
        Tri.GeometryName = geometryNames[g].c_str();
        Tri.pVertexBuffer = pModel->pVertexBuffer;
        Tri.VertexStride = sizeof(Vertex);
        Tri.VertexOffset = 0;
        Tri.VertexCount = vertexCount;
        Tri.VertexValueType = VT_FLOAT32;
        Tri.VertexComponentCount = 3;
        Tri.pIndexBuffer = pModel->pIndexBuffer;
        Tri.IndexType = VT_UINT32;
        // Indices already count from the model's first vertex, so only the start in the index buffer differs
        Tri.IndexOffset = static_cast<Uint64>(submesh.IndexOffset) * sizeof(Uint32);
        Tri.PrimitiveCount = submesh.IndexCount / 3;

        // Flagged opaque, a hit is final without the shader being asked. Cutouts and glass are left for rays to test
        // (rt_scene.hlsli), which costs more, so only they go without the flag.
        const bool opaque = submesh.MaterialIndex >= pModel->PBRMaterials.size()
            || pModel->PBRMaterials[submesh.MaterialIndex].AlphaMode == MaterialAlphaMode::Opaque;
        // A triangle can be reported more than once; for glass that would tint a shadow twice
        Tri.Flags = opaque ? RAYTRACING_GEOMETRY_FLAG_OPAQUE : RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANY_HIT_INVOCATION;
    }

    BuildBLASAttribs BuildAttribs{};
    BuildAttribs.pBLAS = pModel->pBLAS;
    BuildAttribs.pTriangleData = TriData.data();
    BuildAttribs.TriangleDataCount = geometryCount;
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
    m_ScenePool.Clear();
}