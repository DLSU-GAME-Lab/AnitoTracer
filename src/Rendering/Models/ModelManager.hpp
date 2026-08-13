#pragma once
#include "ModelStructs.hpp"

#include "AssetLoading/AssetLoader.hpp"

class ModelManager : public gbe::AssetLoader<Model> {
public:
    static ModelManager& GetInstance() {
        static ModelManager instance;
        instance.AssignSelfAsLoader();
        return instance;
    }

    // Must be called once before loading any models
    void Initialize(IRenderDevice* pDevice, IDeviceContext* mContext);

    // Returns a pointer to the cached model, or loads it if not present
    Model* LoadModel(const std::string& filepath);

    // Returns a cached texture view, or loads it.
    // isSRGB should be true only for color data (e.g. base color/emissive maps).
    // Normal maps, metallic/roughness maps, and AO maps store linear data and
    // must be loaded with isSRGB = false, otherwise the gamma decode curve
    // will distort values (especially near the 0.5 "flat" midpoint used by
    // normal maps), causing incorrect lighting/normals.
    ITextureView* LoadTexture(const std::string& filepath, bool isSRGB = false);

    std::vector<IDeviceObject*> GetCachedTextures() const {
        std::vector<IDeviceObject*> textures;
        textures.reserve(m_TextureCache.size());
        for (const auto& pair : m_TextureCache) {
            textures.push_back(pair.second.RawPtr());
        }
        return textures;
    }

    ITextureView* GetDefaultTexture() const {
        return m_pDefaultTextureView.RawPtr();
    }

    // Clears the cache and releases Vulkan resources
    void ClearCache();

private:
    ModelManager() = default;
    ~ModelManager() { ClearCache(); }
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    void LoadDefaultWhite();

    ITextureView* LoadMaterialTexture(aiMaterial* material, aiTextureType type, const std::string& modelDir, bool& outHasProperty, bool isSRGB = false);

    IRenderDevice* m_pDevice = nullptr;
    IDeviceContext* pContext = nullptr;

    std::unordered_map<std::string, std::unique_ptr<Model>> m_ModelCache;
    std::unordered_map<std::string, RefCntAutoPtr<ITextureView>> m_TextureCache;

    //Default white tex
    RefCntAutoPtr<ITextureView> m_pDefaultTextureView;
};