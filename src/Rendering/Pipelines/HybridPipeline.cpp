#include "HybridPipeline.hpp"
#include "../RenderData.hpp"

bool Diligent::HybridPipeline::SupportsSceneRayShading()
{
    return ModelManager::GetInstance().GetSceneGeometryPool().IsEnabled();
}

void Diligent::HybridPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    SetDevice(pDevice, pSwapChain);
    CreateCommonConstantBuffers();
    InitializeRayTracingResources();

    m_pPSO = CreateModelPSOBuilder("PBR Rendering PSO", "hybrid_ps.hlsl")
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .PixelMacros(GetRayTracingMacros())
        .Variables(GetRayTracingVariables())
        .Samplers(GetRayTracingSamplers())
        .Build(pDevice);
    m_pSRB = PipelineUtils::CreateSRB(m_pPSO);
    BindCommonBuffers(m_pSRB);
    BindRayTracingResources();
}

void Diligent::HybridPipeline::InitializeRayTracingResources()
{
    if (!m_SceneTLAS.IsInitialized()) m_SceneTLAS.Initialize(m_pDevice, 1000, SupportsSceneRayShading());

    // 1x1 black cube so the shader can always sample the sky lighting cubes; sky light simply contributes nothing
    if (!m_pFallbackEnvMap) m_pFallbackEnvMap = PipelineUtils::CreateSolidColorTexture(m_pDevice, "Hybrid Fallback Environment Map", 0xFF000000, true);
}

std::vector<Diligent::ShaderResourceVariableDesc> Diligent::HybridPipeline::GetRayTracingVariables()
{
    std::vector<ShaderResourceVariableDesc> variables = {
        { SHADER_TYPE_PIXEL, "g_TLAS", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        // Dynamic so the sky can be swapped after the fallback is bound
        { SHADER_TYPE_PIXEL, "g_EnvIrradiance", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        { SHADER_TYPE_PIXEL, "g_EnvSpecular", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }
    };

    if (SupportsSceneRayShading()) {
        // Dynamic: these buffers are replaced when they grow (more models, more instances), and are bound again every frame
        variables.push_back({ SHADER_TYPE_PIXEL, "g_SceneVertices", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC });
        variables.push_back({ SHADER_TYPE_PIXEL, "g_SceneIndices", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC });
        variables.push_back({ SHADER_TYPE_PIXEL, "g_SceneGeometry", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC });
        // Mutable: a dynamic variable is written again for every draw, far too much for an array of hundreds of
        // textures in the forward pass. It only changes when a model loads; see BindSceneTextures.
        variables.push_back({ SHADER_TYPE_PIXEL, "g_SceneTextures", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE });
    }
    return variables;
}

std::vector<Diligent::ImmutableSamplerDesc> Diligent::HybridPipeline::GetRayTracingSamplers()
{
    std::vector<ImmutableSamplerDesc> samplers = {
        { SHADER_TYPE_PIXEL, "g_EnvMap_sampler", PipelineUtils::LinearClampSampler() }
    };

    if (SupportsSceneRayShading()) {
        // Ray hits choose the mip themselves (there are no screen-space derivatives at a hit), so plain trilinear is enough
        samplers.push_back({ SHADER_TYPE_PIXEL, "g_SceneTextures_sampler", PipelineUtils::LinearWrapSampler() });
    }
    return samplers;
}

Diligent::ShaderMacroList Diligent::HybridPipeline::GetRayTracingMacros()
{
    if (!SupportsSceneRayShading()) return {};

    return {
        { "RT_SCENE_SHADING", "1" },
        { "SCENE_TEXTURE_COUNT", std::to_string(SceneGeometryPool::kMaxTextures) }
    };
}

void Diligent::HybridPipeline::BindRayTracingResources()
{
    // The TLAS is bound here, so it MUST have a valid state before rendering!
    PipelineUtils::BindVariable(GetRayTracingSRB(), SHADER_TYPE_PIXEL, "g_TLAS", m_SceneTLAS.Get());
    SetSkyLighting(m_pEnvIrradianceSRV, m_pEnvSpecularSRV);

    // A new SRB starts with an empty texture array
    if (SupportsSceneRayShading()) BindSceneTextures();
}

void Diligent::HybridPipeline::BindSceneTextures()
{
    IShaderResourceBinding* pSRB = GetRayTracingSRB();
    IShaderResourceVariable* pTextures = pSRB ? pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_SceneTextures") : nullptr;
    if (!pTextures) return;

    const SceneGeometryPool& pool = ModelManager::GetInstance().GetSceneGeometryPool();
    const std::vector<IDeviceObject*>& textures = pool.GetTextureArray();

    // Every slot is bound (unused ones hold a white texture), since the shader may index any of them
    pTextures->SetArray(textures.data(), 0, static_cast<Uint32>(textures.size()), SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
    m_BoundTextureVersion = pool.GetTextureVersion();
}

void Diligent::HybridPipeline::SetSkyLighting(ITextureView* pIrradianceSRV, ITextureView* pSpecularSRV)
{
    m_pEnvIrradianceSRV = pIrradianceSRV;
    m_pEnvSpecularSRV = pSpecularSRV;
    if (!m_pFallbackEnvMap) return;

    ITextureView* pFallback = m_pFallbackEnvMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    PipelineUtils::BindVariable(GetRayTracingSRB(), SHADER_TYPE_PIXEL, "g_EnvIrradiance", pIrradianceSRV ? pIrradianceSRV : pFallback);
    PipelineUtils::BindVariable(GetRayTracingSRB(), SHADER_TYPE_PIXEL, "g_EnvSpecular", pSpecularSRV ? pSpecularSRV : pFallback);
}

void Diligent::HybridPipeline::SyncSceneRayData(IDeviceContext* pContext)
{
    SceneGeometryPool& pool = ModelManager::GetInstance().GetSceneGeometryPool();
    pool.Sync(pContext);

    IShaderResourceBinding* pSRB = GetRayTracingSRB();
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_SceneVertices", pool.GetVertexSRV());
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_SceneIndices", pool.GetIndexSRV());
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_SceneGeometry", m_SceneTLAS.GetGeometryRecordsSRV());

    if (m_BoundTextureVersion != pool.GetTextureVersion()) {
        // The array is rewritten in place, and frames already sent to the GPU may still be reading it. Wait for them
        // first. This only happens on a frame in which a model with new textures was loaded, which hitches anyway.
        m_pDevice->IdleGPU();
        BindSceneTextures();
    }
}

void Diligent::HybridPipeline::StartFrameRender(IDeviceContext* pContext, const RenderData& renderData)
{
    BasePipeline::StartFrameRender(pContext, renderData);
    m_SceneTLAS.Build(pContext, renderData);

    // After the TLAS: building it can replace the geometry record buffer
    if (SupportsSceneRayShading()) SyncSceneRayData(pContext);
}
