#include "HybridPipeline.hpp"
#include "../RenderData.hpp"
#include <algorithm>
#include <utility>

bool Diligent::HybridPipeline::SupportsSceneRayShading()
{
    return ModelManager::GetInstance().GetSceneGeometryPool().IsEnabled();
}

void Diligent::HybridPipeline::CreatePasses()
{
    InitializeRayTracingResources();
    RebuildRayPipelines();
}

void Diligent::HybridPipeline::RebuildRayPipelines()
{
    BuildMainPass(CreateModelPSOBuilder("PBR Rendering PSO", "hybrid_ps.hlsl")
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .PixelMacros(GetRayTracingMacros())
        .Variables(GetRayTracingVariables())
        .Samplers(GetRayTracingSamplers()));
    InitializeGlassPSO();
    BindRayTracingResources();
}

void Diligent::HybridPipeline::UpdateAlphaRays()
{
    // Rays only read the tables that tell cutouts from solid walls where hits can be shaded at all
    const bool needed = SupportsSceneRayShading() && m_SceneTLAS.HasAlphaGeometry();
    m_FramesWithoutAlphaGeometry = needed ? 0 : m_FramesWithoutAlphaGeometry + 1;

    // Switching off waits a few seconds, so glass that comes and goes doesn't rebuild the passes every time
    constexpr int kFramesBeforeSwitchingOff = 300;
    const bool wanted = needed || (m_AlphaRays && m_FramesWithoutAlphaGeometry < kFramesBeforeSwitchingOff);
    if (wanted == m_AlphaRays) return;

    m_AlphaRays = wanted;
    RebuildRayPipelines();
}

void Diligent::HybridPipeline::InitializeGlassPSO()
{
    // Premultiplied: the shader scales the color it adds by the glass's coverage itself, so that what a window
    // reflects isn't dimmed along with what is behind it
    const RenderTargetBlendDesc blend = PipelineUtils::ColorBlend(BLEND_FACTOR_ONE, BLEND_FACTOR_INV_SRC_ALPHA);

    // Depth is tested against the scene already in the target, but not written: glass doesn't hide glass behind it
    m_Glass = BuildPass(CreateModelPSOBuilder("Glass PSO", "glass_ps.hlsl")
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .PixelMacros(GetRayTracingMacros())
        .Variables(GetRayTracingVariables())
        .Samplers(GetRayTracingSamplers())
        .Depth(true, false, COMPARISON_FUNC_LESS_EQUAL)
        .Blend(0, blend));
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

Diligent::ShaderMacroList Diligent::HybridPipeline::GetRayTracingMacros() const
{
    if (!SupportsSceneRayShading()) return {};

    ShaderMacroList macros = {
        { "RT_SCENE_SHADING", "1" },
        { "SCENE_TEXTURE_COUNT", std::to_string(SceneGeometryPool::kMaxTextures) }
    };
    if (m_AlphaRays) macros.push_back({ "RT_ALPHA_TEST", "1" });
    return macros;
}

void Diligent::HybridPipeline::BindRayTracingResources()
{
    // The TLAS is bound here, so it MUST have a valid state before rendering!
    for (IShaderResourceBinding* pSRB : GetRayTracingSRBs()) {
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_TLAS", m_SceneTLAS.Get());
    }
    SetSkyLighting(m_pEnvIrradianceSRV, m_pEnvSpecularSRV);

    // A new SRB starts with an empty texture array
    if (SupportsSceneRayShading()) BindSceneTextures();
}

void Diligent::HybridPipeline::BindSceneTextures()
{
    const SceneGeometryPool& pool = ModelManager::GetInstance().GetSceneGeometryPool();
    const std::vector<IDeviceObject*>& textures = pool.GetTextureArray();

    for (IShaderResourceBinding* pSRB : GetRayTracingSRBs()) {
        IShaderResourceVariable* pTextures = pSRB ? pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_SceneTextures") : nullptr;
        if (!pTextures) continue;

        // Every slot is bound (unused ones hold a white texture), since the shader may index any of them
        pTextures->SetArray(textures.data(), 0, static_cast<Uint32>(textures.size()), SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
    }
    m_BoundTextureVersion = pool.GetTextureVersion();
}

void Diligent::HybridPipeline::SetSkyLighting(ITextureView* pIrradianceSRV, ITextureView* pSpecularSRV)
{
    m_pEnvIrradianceSRV = pIrradianceSRV;
    m_pEnvSpecularSRV = pSpecularSRV;
    if (!m_pFallbackEnvMap) return;

    ITextureView* pFallback = m_pFallbackEnvMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    for (IShaderResourceBinding* pSRB : GetRayTracingSRBs()) {
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_EnvIrradiance", pIrradianceSRV ? pIrradianceSRV : pFallback);
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_EnvSpecular", pSpecularSRV ? pSpecularSRV : pFallback);
    }
}

void Diligent::HybridPipeline::SyncSceneRayData(IDeviceContext* pContext)
{
    SceneGeometryPool& pool = ModelManager::GetInstance().GetSceneGeometryPool();
    pool.Sync(pContext);

    for (IShaderResourceBinding* pSRB : GetRayTracingSRBs()) {
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_SceneVertices", pool.GetVertexSRV());
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_SceneIndices", pool.GetIndexSRV());
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_SceneGeometry", m_SceneTLAS.GetGeometryRecordsSRV());
    }


    if (m_BoundTextureVersion != pool.GetTextureVersion()) {
        // The array is rewritten in place, and frames already sent to the GPU may still be reading it. Wait for them
        // first. This only happens on a frame in which a model with new textures was loaded, which hitches anyway.
        m_pDevice->IdleGPU();
        BindSceneTextures();
    }
}

void Diligent::HybridPipeline::UpdateProjectSettings(IDeviceContext* /*pContext*/, const ProjectSettings& settings)
{
    m_TransparencySettings = settings.transparency;
}

bool Diligent::HybridPipeline::DrawsGlass() const
{
    return m_TransparencySettings.enabled && m_TransparencySettings.glass;
}

void Diligent::HybridPipeline::FinishFrame(IDeviceContext* pContext, const FrameTarget& target)
{
    RenderGlass(pContext, target);
}

void Diligent::HybridPipeline::RenderGlass(IDeviceContext* pContext, const FrameTarget& target)
{
    // Every ray tracing SRB binds the TLAS, which can only be used once something has built it
    if (!m_Glass || !m_pFrameData || !m_SceneTLAS.IsBuilt()) return;
    if (!m_TransparencySettings.enabled || !m_TransparencySettings.glass) return;

    // Back to front by the distance of each model's middle. Surfaces inside one model keep their order, and models
    // that interleave sort wrongly.
    const glm::vec3 cameraPos = glm::vec3(glm::inverse(m_pFrameData->ViewMatrix)[3]);
    std::vector<std::pair<float, const ModelRenderInstance*>> sorted;
    for (const ModelRenderInstance& instance : m_pFrameData->Models) {
        if ((instance.TransparentSubmeshIndices.empty() && instance.TransmissiveSubmeshIndices.empty()) || !instance.ModelData) continue;

        const glm::vec3 localMiddle = (instance.ModelData->AABBMin + instance.ModelData->AABBMax) * 0.5f;
        const glm::vec3 middle = glm::vec3(instance.WorldTransform * glm::vec4(localMiddle, 1.0f));
        const glm::vec3 offset = middle - cameraPos;
        sorted.emplace_back(glm::dot(offset, offset), &instance);
    }
    if (sorted.empty()) return;
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

    target.Bind(pContext);
    pContext->SetPipelineState(m_Glass.pPSO);
    for (const auto& entry : sorted) {
        RenderModelWithSRB(pContext, *entry.second, /*renderOpaque*/ false, m_Glass.pSRB);
    }
}

void Diligent::HybridPipeline::StartFrameRender(IDeviceContext* pContext, const RenderData& renderData)
{
    m_pFrameData = &renderData;
    BasePipeline::StartFrameRender(pContext, renderData);
    m_SceneTLAS.Build(pContext, renderData, DrawsGlass());
    // Before the scene buffers are bound: the passes it rebuilds bind them too
    UpdateAlphaRays();

    // After the TLAS: building it can replace the geometry record buffer
    if (SupportsSceneRayShading()) SyncSceneRayData(pContext);
}
