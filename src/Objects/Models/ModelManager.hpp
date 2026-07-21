#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include "RenderDevice.h"
#include "DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "Common/interface/BasicMath.hpp"
#include "TextureLoader/interface/TextureUtilities.h"
#include "Graphics/GraphicsEngine/interface/TextureView.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <filesystem> // Add this at the top of your cpp file

using namespace Diligent;

// Standard vertex structure
struct Vertex {
    float3 pos;
    float3 normal;
    float2 uv;
};

// Represents a single part of a model with a specific material
struct SubMesh {
    Uint32 IndexCount = 0;
    Uint32 IndexOffset = 0;
    Uint32 BaseVertex = 0;
    Uint32 MaterialIndex = 0;
};

//Holds PBR properties
struct PBRMaterial {
    RefCntAutoPtr<ITextureView> BaseColor;
    RefCntAutoPtr<ITextureView> MetallicRoughness;
    RefCntAutoPtr<ITextureView> Normal;
    RefCntAutoPtr<ITextureView> AO;
    RefCntAutoPtr<ITextureView> Emissive;

    // PBR Factors (Defaults)
    float4 BaseColorFactor = float4(1.0f, 1.0f, 1.0f, 1.0f);
    float MetallicFactor = 0.0f;
    float RoughnessFactor = 1.0f;
};

// Holds the loaded GPU resources for a complete model
struct Model {
    RefCntAutoPtr<IBuffer> pVertexBuffer;
    RefCntAutoPtr<IBuffer> pIndexBuffer;

    std::vector<SubMesh> SubMeshes;
    std::vector<RefCntAutoPtr<ITextureView>> Materials; // Diffuse SRVs mapped to SubMeshes

    //PBR Mats
    std::vector<PBRMaterial> PBRMaterials;

    //Solid colors and possibly fallbacks
    std::vector<float4> MaterialColors;

    //EZ flag
    bool HasPBRProperties = false;
};

class ModelManager {
public:
    static ModelManager& GetInstance() {
        static ModelManager instance;
        return instance;
    }

    // Must be called once before loading any models
    void Initialize(IRenderDevice* pDevice, const std::string& assetBasePath = "Assets/");

    // Returns a pointer to the cached model, or loads it if not present
    Model* LoadModel(const std::string& filepath);

    // Returns a cached texture view, or loads it
    ITextureView* LoadTexture(const std::string& filepath);

    // Clears the cache and releases Vulkan resources
    void ClearCache();

private:
    ModelManager() = default;
    ~ModelManager() { ClearCache(); }
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    void LoadDefaultWhite();

    ITextureView* LoadMaterialTexture(aiMaterial* material, aiTextureType type, const std::string& modelDir, bool& outHasProperty);

    IRenderDevice* m_pDevice = nullptr;
    std::string m_AssetBasePath;

    std::unordered_map<std::string, std::unique_ptr<Model>> m_ModelCache;
    std::unordered_map<std::string, RefCntAutoPtr<ITextureView>> m_TextureCache;

    //Default white tex
    RefCntAutoPtr<ITextureView> m_pDefaultTextureView;
};