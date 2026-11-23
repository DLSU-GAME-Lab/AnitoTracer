#include "SceneGeometryPool.hpp"
#include "../Pipelines/Utils/PipelineUtils.hpp"
#include <iostream>

static_assert(sizeof(SceneGeometryPool::PoolVertex) == 32, "PoolVertex must match SceneVertex in Shaders/rt_scene.hlsli");

void SceneGeometryPool::Initialize(IRenderDevice* pDevice, bool rayTracingEnabled)
{
    // Ray hits pick their texture per pixel, which needs non-uniform indexing into a texture array
    m_Enabled = rayTracingEnabled && pDevice &&
        pDevice->GetDeviceInfo().Features.ShaderResourceRuntimeArrays == DEVICE_FEATURE_STATE_ENABLED;

    if (!m_Enabled) {
        if (rayTracingEnabled) {
            std::cout << "[Info] This device can't index texture arrays per pixel, so reflections show the sky only (no reflected objects)." << std::endl;
        }
        return;
    }

    m_Vertices.Initialize(pDevice, "Scene Geometry Pool Vertices", sizeof(PoolVertex), 4096);
    m_Indices.Initialize(pDevice, "Scene Geometry Pool Indices", sizeof(Uint32), 16384);

    m_pWhiteTexture = Diligent::PipelineUtils::CreateSolidColorTexture(pDevice, "Scene Texture Array Filler", 0xFFFFFFFF);
    m_TextureArray.assign(kMaxTextures, m_pWhiteTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    ++m_TextureVersion;
}

void SceneGeometryPool::AddModel(Model& model, const std::vector<Vertex>& vertices, const std::vector<Uint32>& indices)
{
    if (!m_Enabled || vertices.empty() || indices.empty()) return;

    model.PoolVertexBase = m_VertexCount;
    model.PoolIndexBase = m_IndexCount;

    m_PendingVertices.reserve(m_PendingVertices.size() + vertices.size());
    for (const Vertex& v : vertices) {
        PoolVertex pv;
        pv.Position = v.pos;
        pv.U = v.uv.x;
        pv.Normal = v.normal;
        pv.V = v.uv.y;
        m_PendingVertices.push_back(pv);
    }
    // Indices stay relative to the model's first vertex; the shader adds the model's base
    m_PendingIndices.insert(m_PendingIndices.end(), indices.begin(), indices.end());

    m_VertexCount += static_cast<Uint32>(vertices.size());
    m_IndexCount += static_cast<Uint32>(indices.size());
}

void SceneGeometryPool::RegisterTexture(ITextureView* pView)
{
    if (!m_Enabled || !pView || m_TextureSlots.count(pView) != 0) return;

    if (m_Textures.size() >= kMaxTextures) {
        if (!m_WarnedTexturesFull) {
            std::cout << "[Warn] More than " << kMaxTextures << " material textures are loaded. Objects using the extra ones "
                << "show their material color instead of the texture in reflections (raise SceneGeometryPool::kMaxTextures)." << std::endl;
            m_WarnedTexturesFull = true;
        }
        return;
    }

    const int slot = static_cast<int>(m_Textures.size());
    m_Textures.emplace_back(pView);
    m_TextureArray[slot] = pView;
    m_TextureSlots.emplace(pView, slot);
    ++m_TextureVersion;
}

int SceneGeometryPool::GetTextureSlot(ITextureView* pView) const
{
    if (!pView) return kNoTexture;

    const auto it = m_TextureSlots.find(pView);
    return it != m_TextureSlots.end() ? it->second : kNoTexture;
}

void SceneGeometryPool::Sync(IDeviceContext* pContext)
{
    if (!m_Enabled || !pContext) return;

    if (!m_PendingVertices.empty()) {
        m_Vertices.Append(pContext, m_PendingVertices.data(), static_cast<Uint32>(m_PendingVertices.size()));
        m_PendingVertices.clear();
        m_PendingVertices.shrink_to_fit();
    }
    if (!m_PendingIndices.empty()) {
        m_Indices.Append(pContext, m_PendingIndices.data(), static_cast<Uint32>(m_PendingIndices.size()));
        m_PendingIndices.clear();
        m_PendingIndices.shrink_to_fit();
    }
}

void SceneGeometryPool::Clear()
{
    if (!m_Enabled) return;

    m_Vertices.Clear();
    m_Indices.Clear();
    m_PendingVertices.clear();
    m_PendingIndices.clear();
    m_VertexCount = 0;
    m_IndexCount = 0;

    m_Textures.clear();
    m_TextureSlots.clear();
    m_TextureArray.assign(kMaxTextures, m_pWhiteTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    ++m_TextureVersion;
}
