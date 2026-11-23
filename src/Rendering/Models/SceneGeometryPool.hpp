#pragma once

#include <unordered_map>
#include <vector>

#include "ModelStructs.hpp"
#include "../Pipelines/Utils/StructuredBufferArray.hpp"

// Everything a ray needs to shade what it hits (Shaders/rt_scene.hlsli): every loaded model's vertices and indices
// in two big buffers, and every material texture in one array. A rasterized draw binds one model's buffers and
// textures, but a ray can land on any model, so the shader must be able to reach all of them at once.
//
// Owned by ModelManager, which adds each model as it loads. This is a copy next to the per-model vertex and index
// buffers that drawing and the acceleration structures keep using.
//
// Only enabled when the device can index a texture array per pixel (ray tracing plus the ShaderResourceRuntimeArrays
// feature). Disabled, every call does nothing and reflections fall back to showing only the sky.
class SceneGeometryPool {
public:
    // Size of g_SceneTextures in Shaders/rt_scene.hlsli (passed to the shader as SCENE_TEXTURE_COUNT).
    // Textures registered after the array is full are simply not sampled by ray hits; those surfaces keep their material color.
    static constexpr Uint32 kMaxTextures = 512;
    static constexpr int kNoTexture = -1;
    static constexpr Uint32 kNotInPool = ~0u;

    // Mirrors SceneVertex in Shaders/rt_scene.hlsli. The UV is split so both vectors stay 16 bytes for the shader.
    struct PoolVertex {
        float3 Position;
        float U = 0.0f;
        float3 Normal;
        float V = 0.0f;
    };

    void Initialize(IRenderDevice* pDevice, bool rayTracingEnabled);
    bool IsEnabled() const { return m_Enabled; }

    // Queues the model's geometry and records where it will sit (Model::PoolVertexBase / PoolIndexBase)
    void AddModel(Model& model, const std::vector<Vertex>& vertices, const std::vector<Uint32>& indices);

    // Gives a material texture a slot in the texture array. Null and already registered textures are ignored.
    void RegisterTexture(ITextureView* pView);
    // Slot of a registered texture, or kNoTexture
    int GetTextureSlot(ITextureView* pView) const;

    // Uploads the geometry queued since the last call. Call once per frame before anything traces rays, outside a render pass.
    void Sync(IDeviceContext* pContext);

    IBufferView* GetVertexSRV() const { return m_Vertices.GetSRV(); }
    IBufferView* GetIndexSRV() const { return m_Indices.GetSRV(); }

    // All kMaxTextures slots, ready for IShaderResourceVariable::SetArray. Unused slots hold a white texture.
    const std::vector<IDeviceObject*>& GetTextureArray() const { return m_TextureArray; }
    // Changes whenever a slot changes, so pipelines know when to bind the array again
    Uint64 GetTextureVersion() const { return m_TextureVersion; }

    // Forgets every model and texture (the models themselves are being released)
    void Clear();

private:
    bool m_Enabled = false;

    Diligent::StructuredBufferArray m_Vertices;
    Diligent::StructuredBufferArray m_Indices;

    // Geometry added since the last Sync
    std::vector<PoolVertex> m_PendingVertices;
    std::vector<Uint32> m_PendingIndices;
    // Elements uploaded plus pending: where the next model starts
    Uint32 m_VertexCount = 0;
    Uint32 m_IndexCount = 0;

    std::vector<RefCntAutoPtr<ITextureView>> m_Textures;        // Keeps the registered textures alive
    std::vector<IDeviceObject*> m_TextureArray;                 // kMaxTextures entries
    std::unordered_map<ITextureView*, int> m_TextureSlots;
    RefCntAutoPtr<ITexture> m_pWhiteTexture;
    Uint64 m_TextureVersion = 0;
    bool m_WarnedTexturesFull = false;
};
