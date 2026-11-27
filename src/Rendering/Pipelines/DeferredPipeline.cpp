#include "DeferredPipeline.hpp"
#include "../RenderData.hpp"

void Diligent::DeferredPipeline::CreatePasses()
{
    // Layout must match the outputs of Shaders/Deferred/gbuffer_ps.hlsl
    m_GBuffer.Configure({
            { "G-Buffer AlbedoMetallic",  TEX_FORMAT_RGBA8_UNORM,  "g_GBufferAlbedo" },
            { "G-Buffer NormalRoughness", TEX_FORMAT_RGBA16_FLOAT, "g_GBufferNormal" },
            { "G-Buffer WorldPos",        TEX_FORMAT_RGBA32_FLOAT, "g_GBufferWorldPos" },
            { "G-Buffer Emissive",        TEX_FORMAT_RGBA16_FLOAT, "g_GBufferEmissive" },
        },
        { "Depth Buffer", TEX_FORMAT_D32_FLOAT, "g_GBufferDepth" });
    // Textures are created by PrepareFrame at the size of the view being drawn

    BuildMainPass(CreateModelPSOBuilder("Deferred G-Buffer PSO", "Deferred/gbuffer_ps.hlsl")
        .RenderTargets(m_GBuffer.GetColorFormats(), m_GBuffer.GetDepthFormat()));

    InitializeLightingPSO();
}

void Diligent::DeferredPipeline::InitializeLightingPSO()
{
    // The editor view always uses this pipeline, so the lighting pass must also write into the multisampled target when MSAA is on.
    // Shading is still per pixel (the G-buffer is single sampled), so geometry edges are not anti-aliased.
    // Copy G-buffer depth to the scene target so later passes can depth-test against geometry.
    m_Lighting = BuildPass(PSOBuilder("Deferred Lighting PSO")
        .Shaders("Deferred/fullscreen_vs.hlsl", "Deferred/simple_deferred_light_ps.hlsl")
        .RenderTargets({ GetColorFormat() }, GetDepthFormat())
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .Depth(true, true, COMPARISON_FUNC_ALWAYS)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferAlbedo", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferNormal", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferWorldPos", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferDepth", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferEmissive", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "LightConstants")
        .Variable(SHADER_TYPE_PIXEL, "ShadowSettings")
        .Variable(SHADER_TYPE_PIXEL, "FogConstants")
        .Sampler(SHADER_TYPE_PIXEL, "g_GBuffer_sampler", PipelineUtils::LinearClampSampler()));

    m_GBuffer.BindToSRB(m_Lighting.pSRB);
}

void Diligent::DeferredPipeline::PrepareFrame(const FrameTarget& target)
{
    // Match the target (the game/editor view resolution, or the back buffer in release builds) so the lighting pass's UVs line up
    if (m_GBuffer.Resize(m_pDevice, target.Width, target.Height)) {
        m_GBuffer.BindToSRB(m_Lighting.pSRB);
    }
}

void Diligent::DeferredPipeline::StartFrameRender(IDeviceContext* pContext, const RenderData& renderData)
{
    // Camera constants, and the G-buffer pass's PSO
    BasePipeline::StartFrameRender(pContext, renderData);

    m_GBuffer.SetRenderTargets(pContext);
    m_GBuffer.Clear(pContext, true);
}

void Diligent::DeferredPipeline::FinishFrame(IDeviceContext* pContext, const FrameTarget& target)
{
    target.Bind(pContext);
    PipelineUtils::DrawFullscreenPass(pContext, m_Lighting.pPSO, m_Lighting.pSRB);
}
