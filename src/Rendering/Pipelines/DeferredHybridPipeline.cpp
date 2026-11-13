#include "DeferredHybridPipeline.hpp"
#include "../RenderData.hpp"
#include <algorithm>
#include <cmath>

namespace {
    // Mirrors cbuffer DeferredHybridConstants in Shaders/DeferredHybrid/lighting_ps.hlsl
    struct DeferredHybridConstants
    {
        glm::mat4 InvViewProj;
        glm::vec4 ViewportSize; // xy: size in pixels, zw: 1 / size
        glm::vec4 ViewParams;   // x: angle one pixel covers, in radians (sizes the texture mip a reflection ray reads)
        glm::vec4 GIParams;     // x: 1 when per-pixel GI is on, y: view pixels per GI pixel (1 or 2), z: GI intensity
    };
    static_assert(sizeof(DeferredHybridConstants) % 16 == 0, "DeferredHybridConstants must be 16-byte aligned for HLSL");
}

void Diligent::DeferredHybridPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    SetDevice(pDevice, pSwapChain);
    CreateCommonConstantBuffers();
    InitializeRayTracingResources();
    if (!m_pDeferredCB) m_pDeferredCB = PipelineUtils::CreateDynamicCB<DeferredHybridConstants>(pDevice, "Deferred Hybrid Constant Buffer");

    // Formats must match Shaders/DeferredHybrid/gbuffer_common.hlsli. Kept across re-initialization (pipeline switch, MSAA toggle).
    if (!m_GBuffer.IsConfigured()) {
        m_GBuffer.Configure({
                { "Deferred Hybrid G-Buffer AlbedoMetallic", TEX_FORMAT_RGBA8_UNORM_SRGB, "g_GBufferAlbedoMetallic" },
                { "Deferred Hybrid G-Buffer Normals",        TEX_FORMAT_RGBA16_FLOAT,     "g_GBufferNormals" },
                { "Deferred Hybrid G-Buffer EmissiveAO",     TEX_FORMAT_RGBA16_FLOAT,     "g_GBufferEmissiveAO" },
                { "Deferred Hybrid G-Buffer Roughness",      TEX_FORMAT_R8_UNORM,         "g_GBufferRoughness" },
            },
            { "Deferred Hybrid G-Buffer Depth", TEX_FORMAT_D32_FLOAT, "g_GBufferDepth" },
            /*growOnly*/ true);
    }

    InitializeDepthPrepassPSO();
    InitializeGBufferPSO();
    // Before the lighting pass: its trace SRB is one of GetRayTracingSRBs, which the lighting setup binds
    InitializeDiffuseGI();
    InitializeLightingPSO();
}

void Diligent::DeferredHybridPipeline::InitializeDiffuseGI()
{
    // Created even while the project uses Sky + AO, so switching the GI method needs no rebuild
    if (!SupportsPerPixelGI()) return;

    m_GI.Initialize(m_pDevice, { GetRayTracingVariables(), GetRayTracingSamplers(), GetRayTracingMacros() });

    // Its trace pass shades ray hits with the same lights as the lighting pass, starting from the same G-buffer
    BindCommonBuffers(m_GI.GetTraceSRB());
    m_GBuffer.BindToSRB(m_GI.GetTraceSRB());
}

void Diligent::DeferredHybridPipeline::InitializeDepthPrepassPSO()
{
    // Depth only, into the G-buffer's own depth. Same vertex shader as the G-buffer pass, so both produce identical depth.
    m_pDepthPrepassPSO = CreateModelPSOBuilder("Deferred Hybrid Depth Prepass PSO", "DeferredHybrid/depth_prepass_ps.hlsl")
        .RenderTargets({}, m_GBuffer.GetDepthFormat())
        .Depth(true, true, COMPARISON_FUNC_LESS)
        .Build(m_pDevice);
    m_pDepthPrepassSRB = PipelineUtils::CreateSRB(m_pDepthPrepassPSO);
    BindCommonBuffers(m_pDepthPrepassSRB);
}

void Diligent::DeferredHybridPipeline::InitializeGBufferPSO()
{
    // The prepass already wrote the nearest depth: only fragments matching it pass, and nothing is rewritten.
    // LESS_EQUAL instead of EQUAL keeps this robust if the two passes' depths ever differ in the last bit.
    // m_pPSO / m_pSRB so BasePipeline::RenderModels binds materials through this pass.
    m_pPSO = CreateModelPSOBuilder("Deferred Hybrid G-Buffer PSO", "DeferredHybrid/gbuffer_ps.hlsl")
        .RenderTargets(m_GBuffer.GetColorFormats(), m_GBuffer.GetDepthFormat())
        .Depth(true, false, COMPARISON_FUNC_LESS_EQUAL)
        .Build(m_pDevice);
    m_pSRB = PipelineUtils::CreateSRB(m_pPSO);
    BindCommonBuffers(m_pSRB);
}

void Diligent::DeferredHybridPipeline::InitializeLightingPSO()
{
    // The lighting pass writes into the frame's target, which is multisampled when MSAA is on.
    // Shading is still per pixel (the G-buffer is single sampled), so geometry edges are not anti-aliased.
    // The shader outputs the G-buffer depth, so later passes (particles, debug lines) are occluded by the scene.
    m_pLightingPSO = PSOBuilder("Deferred Hybrid Lighting PSO")
        .Shaders("Deferred/fullscreen_vs.hlsl", "DeferredHybrid/lighting_ps.hlsl")
        .PixelMacros(GetRayTracingMacros())
        .RenderTargets({ GetColorFormat() }, GetDepthFormat())
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .Cull(CULL_MODE_NONE)
        .Depth(true, true, COMPARISON_FUNC_ALWAYS)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferAlbedoMetallic", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferNormals", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferEmissiveAO", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferRoughness", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferDepth", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        // Dynamic: the GI pass hands over different textures from frame to frame
        .Variable(SHADER_TYPE_PIXEL, "g_GIResult")
        .Variable(SHADER_TYPE_PIXEL, "g_GIGuide")
        .Variable(SHADER_TYPE_PIXEL, "DeferredHybridConstants")
        .Variable(SHADER_TYPE_PIXEL, "LightConstants")
        .Variable(SHADER_TYPE_PIXEL, "ShadowSettings")
        .Variable(SHADER_TYPE_PIXEL, "FogConstants")
        .Variables(GetRayTracingVariables())
        .Samplers(GetRayTracingSamplers())
        .Build(m_pDevice);
    m_pLightingSRB = PipelineUtils::CreateSRB(m_pLightingPSO);

    BindCommonBuffers(m_pLightingSRB);
    PipelineUtils::BindVariable(m_pLightingSRB, SHADER_TYPE_PIXEL, "DeferredHybridConstants", m_pDeferredCB);
    BindRayTracingResources();

    // Re-initialization recreates the SRB, so rebind an existing G-buffer
    m_GBuffer.BindToSRB(m_pLightingSRB);

    if (!m_pNoGITexture) m_pNoGITexture = PipelineUtils::CreateSolidColorTexture(m_pDevice, "Deferred Hybrid No GI", 0x00000000);
}

void Diligent::DeferredHybridPipeline::PrepareFrame(const FrameTarget& target)
{
    m_ViewportWidth = target.Width;
    m_ViewportHeight = target.Height;

    if (m_GBuffer.Resize(m_pDevice, m_ViewportWidth, m_ViewportHeight)) {
        m_GBuffer.BindToSRB(m_pLightingSRB);
        m_GBuffer.BindToSRB(m_GI.GetTraceSRB());
    }
}

void Diligent::DeferredHybridPipeline::SetGBufferViewport(IDeviceContext* pContext)
{
    // Binding the G-buffer resets the viewport to its full (possibly larger) size; draw into the frame-sized corner
    PipelineUtils::SetViewport(pContext, m_ViewportWidth, m_ViewportHeight, m_GBuffer.GetWidth(), m_GBuffer.GetHeight());
}

void Diligent::DeferredHybridPipeline::StartFrameRender(IDeviceContext* pContext, const RenderData& renderData)
{
    // Camera constants + TLAS rebuild
    HybridPipeline::StartFrameRender(pContext, renderData);

    // Kept for FinishFrame, where the GI and lighting passes need the camera the G-buffer was drawn with
    m_ViewProj = renderData.ProjectionMatrix * renderData.ViewMatrix;
    m_CameraPos = renderData.CameraPosition;
    // The projection's Y scale is 1 / tan(fovY / 2), so the view is 2 / scale tall at distance 1, spread over the viewport's rows
    const float projScaleY = renderData.ProjectionMatrix[1][1];
    m_PixelAngle = projScaleY != 0.0f ? 2.0f / (std::abs(projScaleY) * static_cast<float>((std::max)(m_ViewportHeight, 1u))) : 0.0f;

    m_GBuffer.SetRenderTargets(pContext, /*depthOnly*/ true);
    m_GBuffer.Clear(pContext, /*clearColor*/ false);
}

void Diligent::DeferredHybridPipeline::UpdateProjectSettings(IDeviceContext* /*pContext*/, const ProjectSettings& settings)

{
    m_GISettings = settings.gi;
}

void Diligent::DeferredHybridPipeline::RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque)
{
    // Transparent surfaces can't live in a G-buffer (one surface per pixel); they would need a forward pass after lighting
    if (!renderOpaque) return;

    // 1. Depth prepass: resolve visibility before any material sampling
    m_GBuffer.SetRenderTargets(pContext, /*depthOnly*/ true);
    SetGBufferViewport(pContext);
    pContext->SetPipelineState(m_pDepthPrepassPSO);
    for (const ModelRenderInstance& model : renderData.Models) {
        RenderModelWithSRB(pContext, model, renderOpaque, m_pDepthPrepassSRB);
    }

    // 2. G-buffer: one material write per visible pixel. Colors aren't cleared; the lighting pass skips pixels at depth 1.
    m_GBuffer.SetRenderTargets(pContext);
    SetGBufferViewport(pContext);
    pContext->SetPipelineState(m_pPSO);
    BasePipeline::RenderModels(pContext, renderData, renderOpaque);
}

void Diligent::DeferredHybridPipeline::FinishFrame(IDeviceContext* pContext, const FrameTarget& target)
{
    // 3. Per-pixel GI, from the finished G-buffer. Any other GI method leaves the lighting pass on Sky + AO.
    const bool traceGI = m_GISettings.method == GI_METHOD_PER_PIXEL && m_GI.IsInitialized();
    if (traceGI) {
        DiffuseGIPass::View view;
        view.ViewProj = m_ViewProj;
        view.CameraPos = m_CameraPos;
        view.Width = m_ViewportWidth;
        view.Height = m_ViewportHeight;
        m_GI.Render(pContext, view, m_GISettings);
    }
    else {
        // Whatever it had gathered is stale by the time it is switched back on
        m_GI.ResetHistory();
    }

    const bool hasGI = traceGI && m_GI.GetResultSRV() && m_GI.GetGuideSRV();
    ITextureView* pNoGI = m_pNoGITexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE);
    PipelineUtils::BindVariable(m_pLightingSRB, SHADER_TYPE_PIXEL, "g_GIResult", hasGI ? m_GI.GetResultSRV() : pNoGI);
    PipelineUtils::BindVariable(m_pLightingSRB, SHADER_TYPE_PIXEL, "g_GIGuide", hasGI ? m_GI.GetGuideSRV() : pNoGI);

    DeferredHybridConstants constants;
    // Transposed like CameraConstants, so HLSL's mul(clipPos, g_InvViewProj) undoes mul(mul(worldPos, g_View), g_Proj)
    constants.InvViewProj = glm::transpose(glm::inverse(m_ViewProj));
    constants.ViewportSize = glm::vec4(
        static_cast<float>(m_ViewportWidth), static_cast<float>(m_ViewportHeight),
        1.0f / static_cast<float>((std::max)(m_ViewportWidth, 1u)), 1.0f / static_cast<float>((std::max)(m_ViewportHeight, 1u)));
    constants.ViewParams = glm::vec4(m_PixelAngle, 0.0f, 0.0f, 0.0f);
    constants.GIParams = glm::vec4(hasGI ? 1.0f : 0.0f, static_cast<float>(m_GI.GetScale()), (std::max)(m_GISettings.intensity, 0.0f), 0.0f);
    PipelineUtils::WriteCB(pContext, m_pDeferredCB, constants);

    // 4. Lighting: ray-traced shading once per visible pixel, composited over the skybox already in the target
    target.Bind(pContext);


    pContext->SetPipelineState(m_pLightingPSO);
    pContext->CommitShaderResources(m_pLightingSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    PipelineUtils::DrawFullscreenTriangle(pContext);
}
