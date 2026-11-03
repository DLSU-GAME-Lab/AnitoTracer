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

    // Ideally for Skyboxes
    ITextureView* LoadCubemap(const std::string& filepath, bool isSRGB = true);

    // Clears the cache and releases Vulkan resources
    void ClearCache();

    void SetRayTracingEnabled(bool is_enabled) { m_isRayTracingEnabled = is_enabled; }

private:
    ModelManager() = default;
    ~ModelManager() { ClearCache(); }
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    void LoadDefaultWhite();

    ITextureView* LoadMaterialTexture(aiMaterial* material, aiTextureType type, const std::string& modelDir, bool& outHasProperty, bool isSRGB = false);

    // --- Refactored Pipeline Stages ---

    /// <summary>
    /// Processes materials from the Assimp scene and populates the model's PBR properties.
    /// </summary>
    /// <param name="pScene">Pointer to the imported Assimp scene.</param>
    /// <param name="pModel">Pointer to the model being populated.</param>
    /// <param name="modelDir">Directory path of the model for texture loading.</param>
    void ProcessMaterials(const aiScene* pScene, Model* pModel, const std::string& modelDir);

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

    // Default white tex
    RefCntAutoPtr<ITextureView> m_pDefaultTextureView;
};