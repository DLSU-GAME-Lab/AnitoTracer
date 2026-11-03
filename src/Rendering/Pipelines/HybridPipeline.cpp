#include "HybridPipeline.hpp"
#include "../RenderData.hpp"

void Diligent::HybridPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    SetDevice(pDevice, pSwapChain);
    CreateCommonConstantBuffers();
    InitializeRayTracingResources();

    m_pPSO = CreateModelPSOBuilder("PBR Rendering PSO", "hybrid_ps.hlsl")
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .Variables(GetRayTracingVariables())
        .Samplers({ GetEnvironmentMapSampler() })
        .Build(pDevice);
    m_pSRB = PipelineUtils::CreateSRB(m_pPSO);
    BindCommonBuffers(m_pSRB);
    BindRayTracingResources();
}

void Diligent::HybridPipeline::InitializeRayTracingResources()
{
    if (!m_SceneTLAS.IsInitialized()) m_SceneTLAS.Initialize(m_pDevice);

    // 1x1 black cube so the shader can always sample g_EnvMap; sky light simply contributes nothing
    if (!m_pFallbackEnvMap) m_pFallbackEnvMap = PipelineUtils::CreateSolidColorTexture(m_pDevice, "Hybrid Fallback Environment Map", 0xFF000000, true);
}

std::vector<Diligent::ShaderResourceVariableDesc> Diligent::HybridPipeline::GetRayTracingVariables()
{
    return {
        { SHADER_TYPE_PIXEL, "g_TLAS", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC },
        // Dynamic so the skybox can be swapped after the fallback is bound
        { SHADER_TYPE_PIXEL, "g_EnvMap", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }
    };
}

Diligent::ImmutableSamplerDesc Diligent::HybridPipeline::GetEnvironmentMapSampler()
{
    return { SHADER_TYPE_PIXEL, "g_EnvMap_sampler", PipelineUtils::LinearClampSampler() };
}

void Diligent::HybridPipeline::BindRayTracingResources()
{
    // The TLAS is bound here, so it MUST have a valid state before rendering!
    PipelineUtils::BindVariable(GetRayTracingSRB(), SHADER_TYPE_PIXEL, "g_TLAS", m_SceneTLAS.Get());
    SetEnvironmentMap(m_pEnvMapSRV);
}

void Diligent::HybridPipeline::SetEnvironmentMap(ITextureView* pCubemapSRV)
{
    m_pEnvMapSRV = pCubemapSRV;
    if (!m_pFallbackEnvMap) return;

    PipelineUtils::BindVariable(GetRayTracingSRB(), SHADER_TYPE_PIXEL, "g_EnvMap", GetEnvironmentMapView());
}

Diligent::ITextureView* Diligent::HybridPipeline::GetEnvironmentMapView() const
{
    return m_pEnvMapSRV ? m_pEnvMapSRV.RawPtr() : m_pFallbackEnvMap->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
}

void Diligent::HybridPipeline::StartFrameRender(IDeviceContext* pContext, const RenderData& renderData)
{
    BasePipeline::StartFrameRender(pContext, renderData);
    m_SceneTLAS.Build(pContext, renderData);
}
