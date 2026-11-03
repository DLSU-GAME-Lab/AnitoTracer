#include "DeferredPipeline.hpp"
#include "../RenderData.hpp"

void Diligent::DeferredPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    SetDevice(pDevice, pSwapChain);
    CreateCommonConstantBuffers();

    // Layout must match the outputs of Shaders/Deferred/gbuffer_ps.hlsl
    m_GBuffer.Configure({
            { "G-Buffer AlbedoMetallic",  TEX_FORMAT_RGBA8_UNORM,  "g_GBufferAlbedo" },
            { "G-Buffer NormalRoughness", TEX_FORMAT_RGBA16_FLOAT, "g_GBufferNormal" },
            { "G-Buffer WorldPos",        TEX_FORMAT_RGBA32_FLOAT, "g_GBufferWorldPos" },
        },
        { "Depth Buffer", GetDepthFormat(), nullptr });
    // Textures are created by PrepareFrame at the size of the view being drawn

    m_pPSO = CreateModelPSOBuilder("Deferred G-Buffer PSO", "Deferred/gbuffer_ps.hlsl")
        .RenderTargets(m_GBuffer.GetColorFormats(), m_GBuffer.GetDepthFormat())
        .Build(pDevice);
    m_pSRB = PipelineUtils::CreateSRB(m_pPSO);
    BindCommonBuffers(m_pSRB);

    InitializeLightingPSO();
}

void Diligent::DeferredPipeline::InitializeLightingPSO()
{
    // The editor view always uses this pipeline, so the lighting pass must also write into the multisampled target when MSAA is on.
    // Shading is still per pixel (the G-buffer is single sampled), so geometry edges are not anti-aliased.
    m_pLightingPSO = PSOBuilder("Deferred Lighting PSO")
        .Shaders("Deferred/fullscreen_vs.hlsl", "Deferred/simple_deferred_light_ps.hlsl")
        .RenderTargets({ GetColorFormat() }, TEX_FORMAT_UNKNOWN) // No depth testing for lighting pass
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferAlbedo", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferNormal", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferWorldPos", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "LightConstants")
        .Variable(SHADER_TYPE_PIXEL, "ShadowSettings")
        .Variable(SHADER_TYPE_PIXEL, "FogConstants")
        .Sampler(SHADER_TYPE_PIXEL, "g_GBuffer_sampler", PipelineUtils::LinearClampSampler())
        .Build(m_pDevice);
    m_pLightingSRB = PipelineUtils::CreateSRB(m_pLightingPSO);

    BindCommonBuffers(m_pLightingSRB);
    m_GBuffer.BindToSRB(m_pLightingSRB);
}

void Diligent::DeferredPipeline::PrepareFrame(const FrameTarget& target)
{
    // Match the target (the game/editor view resolution, or the back buffer in release builds) so the lighting pass's UVs line up
    if (m_GBuffer.Resize(m_pDevice, target.Width, target.Height)) {
        m_GBuffer.BindToSRB(m_pLightingSRB);
    }
}

void Diligent::DeferredPipeline::StartFrameRender(IDeviceContext* pContext, const RenderData& renderData)
{
    UpdateCameraConstants(pContext, renderData);

    m_GBuffer.SetRenderTargets(pContext);
    m_GBuffer.Clear(pContext, true);

    pContext->SetPipelineState(m_pPSO);
}

void Diligent::DeferredPipeline::FinishFrame(IDeviceContext* pContext, const FrameTarget& target)
{
    ITextureView* pRTV = target.pRTV;
    pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(m_pLightingPSO);
    pContext->CommitShaderResources(m_pLightingSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    PipelineUtils::DrawFullscreenTriangle(pContext);
}
