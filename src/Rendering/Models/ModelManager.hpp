#pragma once
#include "ModelStructs.hpp"

#include "AssetPipeline.hpp"

#include "Types/IModel.h"

class ModelManager : gbe::AssetLoader<IModel> {
public:
    static ModelManager& GetInstance() {
        static ModelManager instance;
        return instance;
    }

    // Must be called once before loading any models
    void Initialize(IRenderDevice* pDevice);

    // Returns a pointer to the cached model, or loads it if not present
    Model* LoadModel(const std::string& filepath);

    // Returns a cached texture view, or loads it
    ITextureView* LoadTexture(const std::string& filepath);

    // Clears the cache and releases Vulkan resources
    void ClearCache();

private:
    ModelManager() = default;
    ~ModelManager() { ClearCache(); }
    ModelManager& operator=(const ModelManager&) = delete;
    ModelManager(const ModelManager&) = delete;

    void LoadDefaultWhite();

    ITextureView* LoadMaterialTexture(aiMaterial* material, aiTextureType type, const std::string& modelDir, bool& outHasProperty);

    IRenderDevice* m_pDevice = nullptr;
    
    std::unordered_map<std::string, std::unique_ptr<Model>> m_ModelCache;
    std::unordered_map<std::string, RefCntAutoPtr<ITextureView>> m_TextureCache;

    //Default white tex
    RefCntAutoPtr<ITextureView> m_pDefaultTextureView;

    //IMPLEMENTED REQUIRED ASSET MANAGER RESPONSIBILITIES
    virtual bool LoadAssetImpl(std::unique_ptr<IModel> fileAsset);
};