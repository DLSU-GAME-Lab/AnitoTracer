#include "DeferredHybridPipeline.hpp"
#include "../RenderData.hpp"
#include <algorithm>
#include <cmath>

namespace {
    using namespace Diligent;

    // Mirrors cbuffer DeferredHybridConstants in Shaders/DeferredHybrid/deferred_common.hlsli
    struct DeferredHybridConstants
    {
        glm::mat4 InvViewProj;
        glm::vec4 ViewportSize; // xy: size in pixels, zw: 1 / size
        glm::vec4 ViewParams;   // x: angle one pixel covers, in radians (sizes the texture mip a reflection ray reads)
        glm::vec4 ViewOrigin;   // xyz: where the camera is; the guide's distances are measured from here
        glm::vec4 GIParams;     // x: 1 when the indirect signal is per-pixel GI (0: ambient occlusion),
                                // y: view pixels per pixel of the indirect signal (1 or 2), z: GI intensity

        uint32_t SignalFrame = 0;   // Counts up every frame: moves the rays of the signals a denoiser averages
        uint32_t DenoiseFlags = 0;  // DENOISE_* bits below
        uint32_t ShadowLayer = 0;   // Shadow trace pass: which four light slots this draw covers
        uint32_t Padding = 0;
    };
    static_assert(sizeof(DeferredHybridConstants) % 16 == 0, "DeferredHybridConstants must be 16-byte aligned for HLSL");

    // Bits of DeferredHybridConstants::DenoiseFlags: the signals that go through a denoiser this frame.
    // Mirror the DENOISE_* constants in Shaders/DeferredHybrid/deferred_common.hlsli
    enum DenoiseFlags : uint32_t {
        DENOISE_SHADOWS = 1u << 0,
        DENOISE_INDIRECT = 1u << 1,
        DENOISE_REFLECTIONS = 1u << 2,
    };

    constexpr TEXTURE_FORMAT kGuideFormat = TEX_FORMAT_RGBA16_FLOAT;
    // A shadow ray is 0 or 1
    constexpr TEXTURE_FORMAT kShadowRaysFormat = TEX_FORMAT_RGBA8_UNORM;
    // HDR light, and for ambient occlusion a direction, whose components can be negative
    constexpr TEXTURE_FORMAT kLightRaysFormat = TEX_FORMAT_RGBA16_FLOAT;

    // How each signal is denoised (SignalDenoiser::Settings). Per-pixel GI takes its two from the project's GI settings.
    //
    // Shadows: react fast (a moving shadow must not trail) and keep hard edges hard; the blur only works where
    // the rays of a penumbra disagree.
    constexpr float kShadowMinBlend = 0.1f;
    constexpr int kShadowFilterPasses = 3;
    constexpr float kShadowClampSigma = 1.5f;
    constexpr float kShadowBrightnessSigma = 3.0f;
    // Per-pixel GI: how long it is averaged and how far it is blurred come from the project's GI settings. The blur
    // narrows where a pixel's rays have settled, which keeps the shading under and beside objects.
    constexpr float kGIBrightnessSigma = 4.0f;
    // Ambient occlusion: soft everywhere, so it can be averaged long and blurred freely
    constexpr float kAOMinBlend = 0.04f;
    constexpr int kAOFilterPasses = 4;
    // At half resolution a pixel is twice as wide, so one pass fewer reaches as far across the screen
    constexpr int kAOHalfResolutionFilterPasses = 3;
    // Reflections: follow what they show closely; the blur widens with roughness and distance on its own
    constexpr float kReflectionMinBlend = 0.1f;
    constexpr int kReflectionFilterPasses = 4;
    constexpr float kReflectionClampSigma = 2.0f;
    constexpr float kReflectionBrightnessSigma = 4.0f;

    RefCntAutoPtr<ITexture> CreateTarget(IRenderDevice* pDevice, const char* name, TEXTURE_FORMAT format, Uint32 width, Uint32 height)
    {
        TextureDesc Desc;
        Desc.Name = name;
        Desc.Type = RESOURCE_DIM_TEX_2D;
        Desc.Width = width;
        Desc.Height = height;
        Desc.Format = format;
        Desc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;

        RefCntAutoPtr<ITexture> pTexture;
        pDevice->CreateTexture(Desc, nullptr, &pTexture);
        return pTexture;
    }

    ITextureView* SRV(ITexture* pTexture) { return pTexture ? pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE) : nullptr; }
}

void Diligent::DeferredHybridPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    SetDevice(pDevice, pSwapChain);
    CreateCommonConstantBuffers();
    InitializeRayTracingResources();
    if (!m_pDeferredCB) m_pDeferredCB = PipelineUtils::CreateDynamicCB<DeferredHybridConstants>(pDevice, "Deferred Hybrid Constant Buffer");
    if (!m_pWhiteTexture) m_pWhiteTexture = PipelineUtils::CreateSolidColorTexture(m_pDevice, "Deferred Hybrid No Shadow", 0xFFFFFFFF);
    if (!m_pBlackTexture) m_pBlackTexture = PipelineUtils::CreateSolidColorTexture(m_pDevice, "Deferred Hybrid No Signal", 0x00000000);

    // Formats must match Shaders/DeferredHybrid/gbuffer_common.hlsli. Kept across re-initialization (pipeline switch, MSAA toggle).
    if (!m_GBuffer.IsConfigured()) {
        m_GBuffer.Configure({
                { "Deferred Hybrid G-Buffer AlbedoMetallic", TEX_FORMAT_RGBA8_UNORM_SRGB, "g_GBufferAlbedoMetallic" },
                { "Deferred Hybrid G-Buffer Normals",        TEX_FORMAT_RGBA16_FLOAT,     "g_GBufferNormals" },
                { "Deferred Hybrid G-Buffer EmissiveAO",     TEX_FORMAT_RGBA16_FLOAT,     "g_GBufferEmissiveAO" },
                { "Deferred Hybrid G-Buffer Roughness",      TEX_FORMAT_R8_UNORM,         "g_GBufferRoughness" },
                { "Deferred Hybrid G-Buffer Velocity",       TEX_FORMAT_RG16_FLOAT,       "g_GBufferVelocity" },
            },
            { "Deferred Hybrid G-Buffer Depth", TEX_FORMAT_D32_FLOAT, "g_GBufferDepth" },
            /*growOnly*/ true);
    }

    InitializeDepthPrepassPSO();
    InitializeGBufferPSO();
    // Before the lighting pass: their SRBs are among GetRayTracingSRBs, which the lighting setup binds
    InitializeDiffuseGI();
    InitializeSignalPasses();
    if (!m_Denoiser.IsInitialized()) m_Denoiser.Initialize(m_pDevice);
    InitializeLightingPSO();
}

void Diligent::DeferredHybridPipeline::InitializeDiffuseGI()
{
    // Created even while the project uses Sky + AO, so switching the GI method needs no rebuild
    if (!SupportsPerPixelGI()) return;

    m_GI.Initialize(m_pDevice, { GetRayTracingVariables(), GetRayTracingSamplers(), GetRayTracingMacros() });

    // Its trace pass shades ray hits with the same lights as the other passes, starting from the same G-buffer
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

Diligent::PSOBuilder Diligent::DeferredHybridPipeline::CreateSignalPassBuilder(const char* name, const char* psFile)
{
    PSOBuilder builder(name);
    builder.Shaders("Deferred/fullscreen_vs.hlsl", psFile)
        .PixelMacros(GetRayTracingMacros())
        .Cull(CULL_MODE_NONE)
        // Mutable: only rebound when the G-buffer is recreated
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferAlbedoMetallic", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferNormals", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferEmissiveAO", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferRoughness", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferDepth", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        // Dynamic: the guide and the signals are other textures from frame to frame
        .Variable(SHADER_TYPE_PIXEL, "g_Guide")
        .Variable(SHADER_TYPE_PIXEL, "g_IndirectSignal")
        .Variable(SHADER_TYPE_PIXEL, "g_ShadowSignal0")
        .Variable(SHADER_TYPE_PIXEL, "g_ShadowSignal1")
        .Variable(SHADER_TYPE_PIXEL, "g_ShadowSignal2")
        .Variable(SHADER_TYPE_PIXEL, "g_ReflectionSignal")
        .Variable(SHADER_TYPE_PIXEL, "DeferredHybridConstants")
        .Variable(SHADER_TYPE_PIXEL, "LightConstants")
        .Variable(SHADER_TYPE_PIXEL, "ShadowSettings")
        .Variable(SHADER_TYPE_PIXEL, "FogConstants")
        .Variables(GetRayTracingVariables())
        .Samplers(GetRayTracingSamplers());
    return builder;
}

void Diligent::DeferredHybridPipeline::BindSignalPassResources(IShaderResourceBinding* pSRB)
{
    BindCommonBuffers(pSRB);
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "DeferredHybridConstants", m_pDeferredCB);
    // Re-initialization recreates the SRB, so rebind an existing G-buffer
    m_GBuffer.BindToSRB(pSRB);
}

void Diligent::DeferredHybridPipeline::InitializeSignalPasses()
{
    m_pGuidePSO = CreateSignalPassBuilder("Deferred Hybrid Guide PSO", "DeferredHybrid/guide_ps.hlsl")
        .RenderTargets({ kGuideFormat }, TEX_FORMAT_UNKNOWN)
        .Build(m_pDevice);
    m_pGuideSRB = PipelineUtils::CreateSRB(m_pGuidePSO);
    BindSignalPassResources(m_pGuideSRB);

    m_pShadowTracePSO = CreateSignalPassBuilder("Deferred Hybrid Shadow Trace PSO", "DeferredHybrid/shadow_trace_ps.hlsl")
        .RenderTargets({ kShadowRaysFormat }, TEX_FORMAT_UNKNOWN)
        .Build(m_pDevice);
    m_pShadowTraceSRB = PipelineUtils::CreateSRB(m_pShadowTracePSO);
    BindSignalPassResources(m_pShadowTraceSRB);

    m_pAOTracePSO = CreateSignalPassBuilder("Deferred Hybrid AO Trace PSO", "DeferredHybrid/ao_trace_ps.hlsl")
        .RenderTargets({ kLightRaysFormat }, TEX_FORMAT_UNKNOWN)
        .Build(m_pDevice);
    m_pAOTraceSRB = PipelineUtils::CreateSRB(m_pAOTracePSO);
    BindSignalPassResources(m_pAOTraceSRB);

    m_pReflectionTracePSO = CreateSignalPassBuilder("Deferred Hybrid Reflection Trace PSO", "DeferredHybrid/reflection_trace_ps.hlsl")
        .RenderTargets({ kLightRaysFormat }, TEX_FORMAT_UNKNOWN)
        .Build(m_pDevice);
    m_pReflectionTraceSRB = PipelineUtils::CreateSRB(m_pReflectionTracePSO);
    BindSignalPassResources(m_pReflectionTraceSRB);
}

void Diligent::DeferredHybridPipeline::InitializeLightingPSO()
{
    // The lighting pass writes into the frame's target, which is multisampled when MSAA is on.
    // Shading is still per pixel (the G-buffer is single sampled), so geometry edges are not anti-aliased.
    // The shader outputs the G-buffer depth, so later passes (particles, debug lines) are occluded by the scene.
    // It traces no rays, but lights with the sky cubes, which come with the ray tracing variables.
    m_pLightingPSO = CreateSignalPassBuilder("Deferred Hybrid Lighting PSO", "DeferredHybrid/lighting_ps.hlsl")
        .RenderTargets({ GetColorFormat() }, GetDepthFormat())
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .Depth(true, true, COMPARISON_FUNC_ALWAYS)
        .Build(m_pDevice);
    m_pLightingSRB = PipelineUtils::CreateSRB(m_pLightingPSO);
    BindSignalPassResources(m_pLightingSRB);

    // Every SRB of GetRayTracingSRBs exists by now
    BindRayTracingResources();
}

std::vector<Diligent::IShaderResourceBinding*> Diligent::DeferredHybridPipeline::GetRayTracingSRBs()
{
    return { m_pLightingSRB, m_GI.GetTraceSRB(), m_pShadowTraceSRB, m_pAOTraceSRB, m_pReflectionTraceSRB };
}

std::vector<Diligent::IShaderResourceBinding*> Diligent::DeferredHybridPipeline::GetGBufferSRBs()
{
    return { m_pLightingSRB, m_GI.GetTraceSRB(), m_pGuideSRB, m_pShadowTraceSRB, m_pAOTraceSRB, m_pReflectionTraceSRB };
}

void Diligent::DeferredHybridPipeline::PrepareFrame(const FrameTarget& target)
{
    m_ViewportWidth = target.Width;
    m_ViewportHeight = target.Height;

    if (m_GBuffer.Resize(m_pDevice, m_ViewportWidth, m_ViewportHeight)) {
        for (IShaderResourceBinding* pSRB : GetGBufferSRBs()) {
            m_GBuffer.BindToSRB(pSRB);
        }
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

    // Kept for FinishFrame, where the passes after the G-buffer need the camera it was drawn with
    m_ViewProj = renderData.ProjectionMatrix * renderData.ViewMatrix;
    // From the view matrix itself, so it can't disagree with what was drawn
    m_CameraPos = glm::vec3(glm::inverse(renderData.ViewMatrix)[3]);
    // The projection's Y scale is 1 / tan(fovY / 2), so the view is 2 / scale tall at distance 1, spread over the viewport's rows
    const float projScaleY = renderData.ProjectionMatrix[1][1];
    m_PixelAngle = projScaleY != 0.0f ? 2.0f / (std::abs(projScaleY) * static_cast<float>((std::max)(m_ViewportHeight, 1u))) : 0.0f;

    m_GBuffer.SetRenderTargets(pContext, /*depthOnly*/ true);
    m_GBuffer.Clear(pContext, /*clearColor*/ false);
}

void Diligent::DeferredHybridPipeline::UpdateLights(IDeviceContext* pContext, const LightConstants& lights)
{
    ScenePipeline::UpdateLights(pContext, lights);

    // The shadow pass is drawn once per four lights
    m_NumDirLights = glm::clamp(lights.NumDirLights, 0, static_cast<int>(MAX_DIR_LIGHTS));
    m_NumPointLights = glm::clamp(lights.NumPointLights, 0, static_cast<int>(MAX_POINT_LIGHTS));
}

void Diligent::DeferredHybridPipeline::UpdateShadowSettings(IDeviceContext* pContext, const ProjectSettings& settings, uint32_t noiseFrame)
{
    ScenePipeline::UpdateShadowSettings(pContext, settings, noiseFrame);
    m_NoiseFrame = noiseFrame;
}

void Diligent::DeferredHybridPipeline::UpdateProjectSettings(IDeviceContext* /*pContext*/, const ProjectSettings& settings)
{
    m_GISettings = settings.gi;
    m_DenoiseSettings = settings.denoise;
    m_ReflectionSettings = settings.reflections;
    m_ReflectionIntensity = settings.lighting.reflection_intensity;
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

    // 2. G-buffer: one material write per visible pixel. Colors aren't cleared; later passes skip pixels at depth 1.
    m_GBuffer.SetRenderTargets(pContext);
    SetGBufferViewport(pContext);
    pContext->SetPipelineState(m_pPSO);
    BasePipeline::RenderModels(pContext, renderData, renderOpaque);
}

void Diligent::DeferredHybridPipeline::ResetSignalHistory()
{
    for (SignalDenoiser::History& history : m_ShadowHistory) {
        history.Reset();
    }
    m_IndirectHistory.Reset();
    m_ReflectionHistory.Reset();
    m_GI.ResetHistory();
}

void Diligent::DeferredHybridPipeline::PrepareSignalTargets(Uint32 width, Uint32 height)
{
    if (width == m_SignalWidth && height == m_SignalHeight && m_pGuide[0] && m_pGuide[1] && m_pReflectionRays) return;

    m_SignalWidth = width;
    m_SignalHeight = height;

    m_pGuide[0] = CreateTarget(m_pDevice, "Deferred Hybrid Guide 0", kGuideFormat, width, height);
    m_pGuide[1] = CreateTarget(m_pDevice, "Deferred Hybrid Guide 1", kGuideFormat, width, height);
    m_pReflectionRays = CreateTarget(m_pDevice, "Deferred Hybrid Reflection Rays", kLightRaysFormat, width, height);
    // Created again when a frame needs them
    for (RefCntAutoPtr<ITexture>& pShadowRays : m_pShadowRays) {
        pShadowRays.Release();
    }
    m_pAORays.Release();

    // Nothing gathered at the old size can be reused
    m_GuideValid = false;
    ResetSignalHistory();
}

void Diligent::DeferredHybridPipeline::DrawSignalPass(IDeviceContext* pContext, IPipelineState* pPSO, IShaderResourceBinding* pSRB, ITexture* pTarget)
{
    // The target is as large as the pass's output, so the viewport binding it sets is the right one
    ITextureView* pRTV = pTarget->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET);
    pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    pContext->SetPipelineState(pPSO);
    pContext->CommitShaderResources(pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    PipelineUtils::DrawFullscreenTriangle(pContext);
}

void Diligent::DeferredHybridPipeline::FinishFrame(IDeviceContext* pContext, const FrameTarget& target)
{
    const Uint32 width = m_ViewportWidth;
    const Uint32 height = m_ViewportHeight;
    if (width == 0 || height == 0) return;

    // The scene has never held anything to trace against: the G-buffer is empty too, so the sky already in the
    // target is the whole picture. (Every pass below binds the acceleration structure, which must be built first.)
    if (!m_SceneTLAS.IsBuilt()) {
        m_GuideValid = false;
        ResetSignalHistory();
        return;
    }

    PrepareSignalTargets(width, height);
    if (!m_pGuide[0] || !m_pGuide[1] || !m_pReflectionRays || !m_pGuidePSO || !m_pShadowTracePSO || !m_pAOTracePSO
        || !m_pReflectionTracePSO || !m_pLightingPSO) return;

    // Which signals go through the denoiser (the project's Denoising settings). The others are shown as traced.
    const bool denoise = m_DenoiseSettings.enabled && m_Denoiser.IsInitialized();
    const bool denoiseShadows = denoise && m_DenoiseSettings.shadows;
    const bool denoiseIndirect = denoise && m_DenoiseSettings.indirect;
    const bool denoiseReflections = denoise && m_DenoiseSettings.reflections;

    // Per-pixel GI, where the project asks for it and the device can run it; any other GI method is Sky + AO
    const bool perPixelGI = m_GISettings.method == GI_METHOD_PER_PIXEL && m_GI.IsInitialized();
    // View pixels per pixel of the indirect signal, along each axis: each method has its own half resolution switch
    const Uint32 indirectScale = perPixelGI ? DiffuseGIPass::GetScale(m_GISettings) : (m_GISettings.ao_half_resolution ? 2u : 1u);

    DeferredHybridConstants constants;
    // Transposed like CameraConstants, so HLSL's mul(clipPos, g_InvViewProj) undoes mul(mul(worldPos, g_View), g_Proj)
    constants.InvViewProj = glm::transpose(glm::inverse(m_ViewProj));
    constants.ViewportSize = glm::vec4(static_cast<float>(width), static_cast<float>(height), 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height));
    constants.ViewParams = glm::vec4(m_PixelAngle, 0.0f, 0.0f, 0.0f);
    constants.ViewOrigin = glm::vec4(m_CameraPos, 1.0f);
    constants.GIParams = glm::vec4(perPixelGI ? 1.0f : 0.0f, static_cast<float>(indirectScale), (std::max)(m_GISettings.intensity, 0.0f), 0.0f);
    constants.SignalFrame = ++m_SignalFrame;
    if (denoiseShadows) constants.DenoiseFlags |= DENOISE_SHADOWS;
    if (denoiseIndirect) constants.DenoiseFlags |= DENOISE_INDIRECT;
    if (denoiseReflections) constants.DenoiseFlags |= DENOISE_REFLECTIONS;
    PipelineUtils::WriteCB(pContext, m_pDeferredCB, constants);

    // 3. Guide. Last frame's stays in the other texture of the pair.
    const Uint32 previousGuide = m_GuideCurrent;
    const Uint32 currentGuide = 1 - m_GuideCurrent;
    DrawSignalPass(pContext, m_pGuidePSO, m_pGuideSRB, m_pGuide[currentGuide]);
    ITextureView* pGuide = SRV(m_pGuide[currentGuide]);
    ITextureView* pPrevGuide = m_GuideValid ? SRV(m_pGuide[previousGuide]) : nullptr;

    SignalDenoiser::Frame frame;
    frame.ViewProj = m_ViewProj;
    frame.ViewProjNoJitter = m_ViewProjNoJitter;
    frame.PrevViewProjNoJitter = m_PrevViewProjNoJitter;
    frame.CameraPos = m_CameraPos;
    frame.PrevCameraPos = m_PrevCameraPos;
    frame.Width = width;
    frame.Height = height;
    frame.PixelAngle = m_PixelAngle;
    frame.pGuide = pGuide;
    frame.pPrevGuide = pPrevGuide;
    frame.pVelocity = GetVelocitySRV();
    frame.HistoryValid = m_GuideValid;

    // 4a. Shadows: one ray per light, four lights per texture. Lights are numbered directional first, then point,
    //     so adding or removing one moves the others to other channels: what was gathered no longer fits.
    if (m_NumDirLights != m_HistoryDirLights || m_NumPointLights != m_HistoryPointLights) {
        for (SignalDenoiser::History& history : m_ShadowHistory) {
            history.Reset();
        }
        m_HistoryDirLights = m_NumDirLights;
        m_HistoryPointLights = m_NumPointLights;
    }

    SignalDenoiser::Settings shadowSettings;
    shadowSettings.SignalKind = SignalDenoiser::SIGNAL_VISIBILITY;
    shadowSettings.MinBlend = kShadowMinBlend;
    shadowSettings.FilterPasses = kShadowFilterPasses;
    shadowSettings.ClampSigma = kShadowClampSigma;
    shadowSettings.BrightnessSigma = kShadowBrightnessSigma;

    const Uint32 lightSlots = static_cast<Uint32>(m_NumDirLights + m_NumPointLights);
    const Uint32 shadowLayers = (std::min)((lightSlots + 3) / 4, kMaxShadowLayers);
    ITextureView* pShadows[kMaxShadowLayers];
    for (Uint32 layer = 0; layer < kMaxShadowLayers; ++layer) {
        pShadows[layer] = SRV(m_pWhiteTexture);

        if (layer < shadowLayers && !m_pShadowRays[layer]) {
            m_pShadowRays[layer] = CreateTarget(m_pDevice, "Deferred Hybrid Shadow Rays", kShadowRaysFormat, width, height);
        }
        if (layer >= shadowLayers || !m_pShadowRays[layer]) {
            m_ShadowHistory[layer].Reset();
            continue;
        }

        constants.ShadowLayer = layer;
        PipelineUtils::WriteCB(pContext, m_pDeferredCB, constants);
        DrawSignalPass(pContext, m_pShadowTracePSO, m_pShadowTraceSRB, m_pShadowRays[layer]);

        pShadows[layer] = SRV(m_pShadowRays[layer]);
        if (denoiseShadows) {
            pShadows[layer] = m_Denoiser.Denoise(pContext, frame, m_ShadowHistory[layer], pShadows[layer], 1, shadowSettings);
        }
        else {
            m_ShadowHistory[layer].Reset();
        }
    }

    // 4b. Indirect light: per-pixel GI rays, or one ambient occlusion ray for Sky + AO. The two fill the same
    //     texture with different things, so a history gathered for one is no use to the other.
    if (static_cast<int>(perPixelGI) != m_HistoryIndirectMethod || indirectScale != m_HistoryIndirectScale) {
        m_IndirectHistory.Reset();
        m_HistoryIndirectMethod = static_cast<int>(perPixelGI);
        m_HistoryIndirectScale = indirectScale;
    }

    SignalDenoiser::Settings indirectSettings;
    indirectSettings.SignalKind = SignalDenoiser::SIGNAL_LIGHT;
    ITextureView* pIndirect = nullptr;
    if (perPixelGI) {
        DiffuseGIPass::View view;
        view.ViewProj = m_ViewProj;
        view.CameraPos = m_CameraPos;
        view.Width = width;
        view.Height = height;
        view.pPrevGuide = pPrevGuide;
        // The GI that was shown last frame, for light that bounces on. Without a denoiser the pass reads its own rays.
        view.pPrevResult = denoiseIndirect ? m_IndirectHistory.GetResultSRV() : nullptr;
        view.Denoised = denoiseIndirect;
        view.NoiseFrame = m_NoiseFrame;
        pIndirect = m_GI.Trace(pContext, view, m_GISettings);

        indirectSettings.MinBlend = 1.0f - glm::clamp(m_GISettings.history_blend, 0.0f, 0.99f);
        indirectSettings.FilterPasses = m_GISettings.filter_passes;
        indirectSettings.BrightnessSigma = kGIBrightnessSigma;
    }
    else {
        // Whatever the GI pass had is stale by the time it is switched back on
        m_GI.ResetHistory();

        // The rays' texture is as large as the signal, so it is made again when the half resolution switch changes
        const Uint32 aoWidth = (width + indirectScale - 1) / indirectScale;
        const Uint32 aoHeight = (height + indirectScale - 1) / indirectScale;
        if (!m_pAORays || m_pAORays->GetDesc().Width != aoWidth || m_pAORays->GetDesc().Height != aoHeight) {
            m_pAORays = CreateTarget(m_pDevice, "Deferred Hybrid AO Rays", kLightRaysFormat, aoWidth, aoHeight);
        }
        if (m_pAORays) {
            DrawSignalPass(pContext, m_pAOTracePSO, m_pAOTraceSRB, m_pAORays);
            pIndirect = SRV(m_pAORays);
        }

        indirectSettings.MinBlend = kAOMinBlend;
        indirectSettings.FilterPasses = indirectScale > 1 ? kAOHalfResolutionFilterPasses : kAOFilterPasses;
    }

    if (denoiseIndirect && pIndirect) {
        pIndirect = m_Denoiser.Denoise(pContext, frame, m_IndirectHistory, pIndirect, indirectScale, indirectSettings);
    }
    else {
        m_IndirectHistory.Reset();
    }
    if (!pIndirect) pIndirect = SRV(m_pBlackTexture);

    // 4c. Reflections: one ray per pixel smooth enough to get one. What a ray hits is lit by the indirect light
    //     gathered at its pixel, so this comes after 4b. Skipped when the lighting pass wouldn't read the result.
    ITextureView* pReflection = SRV(m_pBlackTexture);
    if (m_ReflectionSettings.enabled && m_ReflectionIntensity > 0.0f) {
        PipelineUtils::BindVariable(m_pReflectionTraceSRB, SHADER_TYPE_PIXEL, "g_Guide", pGuide);
        PipelineUtils::BindVariable(m_pReflectionTraceSRB, SHADER_TYPE_PIXEL, "g_IndirectSignal", pIndirect);
        DrawSignalPass(pContext, m_pReflectionTracePSO, m_pReflectionTraceSRB, m_pReflectionRays);
        pReflection = SRV(m_pReflectionRays);

        if (denoiseReflections) {
            SignalDenoiser::Settings reflectionSettings;
            reflectionSettings.SignalKind = SignalDenoiser::SIGNAL_REFLECTION;
            reflectionSettings.MinBlend = kReflectionMinBlend;
            reflectionSettings.FilterPasses = kReflectionFilterPasses;
            reflectionSettings.ClampSigma = kReflectionClampSigma;
            reflectionSettings.BrightnessSigma = kReflectionBrightnessSigma;
            // The limit the shaders trace up to, see BuildShadowSettings
            reflectionSettings.MaxRoughness = glm::clamp(m_ReflectionSettings.max_roughness, 0.01f, 1.0f);
            reflectionSettings.VirtualMotion = true;
            pReflection = m_Denoiser.Denoise(pContext, frame, m_ReflectionHistory, pReflection, 1, reflectionSettings);
        }
        else {
            m_ReflectionHistory.Reset();
        }
    }
    else {
        m_ReflectionHistory.Reset();
    }

    // 5. Lighting: composes each visible pixel from the G-buffer and the signals, over the skybox already in the target
    PipelineUtils::BindVariable(m_pLightingSRB, SHADER_TYPE_PIXEL, "g_Guide", pGuide);
    PipelineUtils::BindVariable(m_pLightingSRB, SHADER_TYPE_PIXEL, "g_IndirectSignal", pIndirect);
    PipelineUtils::BindVariable(m_pLightingSRB, SHADER_TYPE_PIXEL, "g_ShadowSignal0", pShadows[0]);
    PipelineUtils::BindVariable(m_pLightingSRB, SHADER_TYPE_PIXEL, "g_ShadowSignal1", pShadows[1]);
    PipelineUtils::BindVariable(m_pLightingSRB, SHADER_TYPE_PIXEL, "g_ShadowSignal2", pShadows[2]);
    PipelineUtils::BindVariable(m_pLightingSRB, SHADER_TYPE_PIXEL, "g_ReflectionSignal", pReflection);

    target.Bind(pContext);

    pContext->SetPipelineState(m_pLightingPSO);
    pContext->CommitShaderResources(m_pLightingSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    PipelineUtils::DrawFullscreenTriangle(pContext);

    // This frame becomes "last frame"
    m_GuideCurrent = currentGuide;
    m_GuideValid = true;
    m_PrevCameraPos = m_CameraPos;
}
