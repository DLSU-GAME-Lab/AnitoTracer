#pragma once
#include "ModelStructs.hpp"
#include "SceneGeometryPool.hpp"
#include "AssetLoading/AssetLoader.hpp"

class ModelManager : public gbe::AssetLoader<Model> {
public:
    static ModelManager& GetInstance() {
        static ModelManager instance;
        instance.AssignSelfAsLoader();
        return instance;
    }

    // Must be called once before loading any models
    // isRayTracingEnabled dictates if BLAS building and buffer RT flags are applied.
    void Initialize(IRenderDevice* pDevice, IDeviceContext* mContext, bool isRayTracingEnabled = false);

    // Returns a pointer to the cached model, or loads it if not present.
    // Creates standard buffers but defers Ray Tracing BLAS generation.
    Model* LoadModel(const std::string& filepath);

    // Hardware Ray Tracing stage: Builds the Bottom-Level Acceleration Structure (BLAS)
    // Call this after loading if the model will be used in a ray tracing pipeline.
    void BuildBLAS(Model* pModel);

    // Returns a cached texture view, or loads it.
    // isSRGB should be true only for color data (e.g. base color/emissive maps).
    // Normal maps, metallic/roughness maps, and AO maps store linear data and
    // must be loaded with isSRGB = false, otherwise the gamma decode curve
    // will distort values (especially near the 0.5 "flat" midpoint used by
    // normal maps), causing incorrect lighting/normals.
    ITextureView* LoadTexture(const std::string& filepath, bool isSRGB = false);

    // True when a texture loaded by LoadTexture has see-through texels (a fence's gaps, the area around a leaf).
    // Judged when it was loaded. Tells cutout materials from opaque ones in formats that don't say.
    bool HasCutoutAlpha(ITextureView* pTexture) const;

    // Ideally for Skyboxes
    ITextureView* LoadCubemap(const std::string& filepath, bool isSRGB = true);

    // Clears the cache and releases Vulkan resources
    void ClearCache();

    void SetRayTracingEnabled(bool is_enabled) { m_isRayTracingEnabled = is_enabled; }

    // Every loaded model's geometry and material textures in the form ray hits read them (shaded reflections)
    SceneGeometryPool& GetSceneGeometryPool() { return m_ScenePool; }

private:
    ModelManager() = default;
    ~ModelManager() { ClearCache(); }
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    void LoadDefaultWhite();

    // A material's texture of one kind: a file next to the model, or an image embedded in it (GLB files, glTF data
    // URIs), which assimp names "*0", "*1" and so on. modelPath tells embedded images of different models apart.
    ITextureView* LoadMaterialTexture(const aiScene* pScene, const std::string& modelPath, aiMaterial* material, aiTextureType type,
        const std::string& modelDir, bool& outHasProperty, bool isSRGB = false);

    // An image stored inside the model file as PNG or JPEG data. name keeps it apart from the other images in the cache.
    ITextureView* LoadEmbeddedTexture(const aiTexture* pEmbedded, const std::string& name, bool isSRGB);

    // Creates the texture a loader prepared, notes whether it has gaps (HasCutoutAlpha) and caches it under cacheKey
    ITextureView* FinishTexture(ITextureLoader* pLoader, std::string cacheKey, const std::string& label);

    // A material's bump texture (OBJ's map_bump / bump, in assimp's "height" slot). Some models put a normal map there,
    // others a height map. A normal map is loaded like any other; a height map, told apart by being gray, is turned into
    // the normal map the shaders read.
    ITextureView* LoadBumpTexture(const aiScene* pScene, const std::string& modelPath, aiMaterial* material,
        const std::string& modelDir, bool& outHasProperty);

    // The normal map of the height map a loader has read (RGBA8, gray). bumpScale: the material's bump multiplier.
    ITextureView* CreateNormalMapFromHeight(ITextureLoader* pHeight, float bumpScale, std::string cacheKey, const std::string& label);

    // --- Refactored Pipeline Stages ---

    /// <summary>
    /// Processes materials from the Assimp scene and populates the model's PBR properties.
    /// </summary>
    /// <param name="pScene">Pointer to the imported Assimp scene.</param>
    /// <param name="pModel">Pointer to the model being populated.</param>
    /// <param name="modelDir">Directory path of the model for texture loading.</param>
    void ProcessMaterials(const aiScene* pScene, Model* pModel, const std::string& modelDir, const std::string& modelPath);

    /// <summary>
    /// Extracts vertices and indices from the Assimp scene meshes and calculates bounding boxes.
    /// </summary>
    /// <param name="pScene">Pointer to the imported Assimp scene.</param>
    /// <param name="pModel">Pointer to the model being populated.</param>
    /// <param name="outVertices">Output vector containing the aggregated vertex data.</param>
    /// <param name="outIndices">Output vector containing the aggregated index data.</param>
    void ProcessMeshes(const aiScene* pScene, Model* pModel, std::vector<Vertex>& outVertices, std::vector<Uint32>& outIndices);

    /// <summary>
    /// Allocates and initializes Diligent hardware buffers for the model's geometry.
    /// </summary>
    /// <param name="pModel">Pointer to the model receiving the hardware buffers.</param>
    /// <param name="vertices">Vector containing the populated vertex data.</param>
    /// <param name="indices">Vector containing the populated index data.</param>
    void CreateHardwareBuffers(Model* pModel, const std::vector<Vertex>& vertices, const std::vector<Uint32>& indices);

    IRenderDevice* m_pDevice = nullptr;
    IDeviceContext* pContext = nullptr;
    bool m_isRayTracingEnabled = false;

    std::unordered_map<std::string, std::unique_ptr<Model>> m_ModelCache;
    std::unordered_map<std::string, RefCntAutoPtr<ITextureView>> m_TextureCache;
    std::unordered_set<const ITextureView*> m_CutoutTextures;

    // Default white tex
    RefCntAutoPtr<ITextureView> m_pDefaultTextureView;

    SceneGeometryPool m_ScenePool;
};