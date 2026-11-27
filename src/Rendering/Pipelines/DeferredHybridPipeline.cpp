#include "DeferredHybridPipeline.hpp"
#include "../RenderData.hpp"
#include <algorithm>
#include <cmath>

namespace {
    using namespace Diligent;
    using PipelineUtils::CreateRenderTarget;
    using PipelineUtils::RTV;
    using PipelineUtils::SRV;

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
        uint32_t GlassFlags = 0;    // GLASS_* bits below
    };
    static_assert(sizeof(DeferredHybridConstants) % 16 == 0, "DeferredHybridConstants must be 16-byte aligned for HLSL");

    // Bits of DeferredHybridConstants::DenoiseFlags: the signals that go through a denoiser this frame.
    // Mirror the DENOISE_* constants in Shaders/DeferredHybrid/deferred_common.hlsli
    enum DenoiseFlags : uint32_t {
        DENOISE_SHADOWS = 1u << 0,
        DENOISE_INDIRECT = 1u << 1,
        DENOISE_REFLECTIONS = 1u << 2,
    };

    // Bits of DeferredHybridConstants::GlassFlags. Mirror the GLASS_* constants in Shaders/DeferredHybrid/deferred_common.hlsli
    constexpr uint32_t GLASS_TINT_SHADOWS = 1u;

    constexpr TEXTURE_FORMAT kGuideFormat = TEX_FORMAT_RGBA16_FLOAT;
    // Glass tints light by a color, which can be brighter than white in one channel
    constexpr TEXTURE_FORMAT kGlassTintFormat = TEX_FORMAT_RGBA16_FLOAT;
    // A shadow ray is 0 or 1
    constexpr TEXTURE_FORMAT kShadowRaysFormat = TEX_FORMAT_RGBA8_UNORM;
    // HDR light, and for ambient occlusion a direction, whose components can be negative
    constexpr TEXTURE_FORMAT kLightRaysFormat = TEX_FORMAT_RGBA16_FLOAT;

    // Most render targets a signal pass writes at once (the shadow pass that also writes the glass tint)
    constexpr Uint32 kMaxSignalTargets = 2;

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
    // The glass tint: like shadows, it follows glass that moves and keeps its edges, but it is smooth everywhere else
    constexpr float kGlassTintMinBlend = 0.1f;
    constexpr int kGlassTintFilterPasses = 2;
}

// One frame of FinishFrame: what is decided at its start, and what each pass found for the lighting pass
struct Diligent::DeferredHybridPipeline::SignalFrame
{
    Uint32 Width = 0;
    Uint32 Height = 0;

    // Which signals go through the denoiser (the project's Denoising settings). The others are shown as traced.
    bool DenoiseShadows = false;
    bool DenoiseIndirect = false;
    bool DenoiseReflections = false;
    // Light that crosses glass takes its color (see TraceShadows)
    bool TintShadows = false;
    // Per-pixel GI, where the project asks for it and the device can run it; any other GI method is Sky + AO
    bool PerPixelGI = false;
    // View pixels per pixel of the indirect signal, along each axis
    Uint32 IndirectScale = 1;

    // What every pass reads. The shadow passes change ShadowLayer as they go.
    DeferredHybridConstants Constants;
    // The camera and guide the denoiser needs
    SignalDenoiser::Frame Denoise;
    Uint32 CurrentGuide = 0;                // The m_pGuide entry written this frame
    ITextureView* pGuide = nullptr;
    ITextureView* pPrevGuide = nullptr;     // Null until a frame has been drawn at this size

    // What the passes found, as the lighting pass reads them
    ITextureView* pShadows[kMaxShadowLayers] = {};
    ITextureView* pGlassTint = nullptr;
    ITextureView* pIndirect = nullptr;
    ITextureView* pReflection = nullptr;
};

void Diligent::DeferredHybridPipeline::CreatePasses()
{
    InitializeRayTracingResources();
    if (!m_pDeferredCB) m_pDeferredCB = PipelineUtils::CreateDynamicCB<DeferredHybridConstants>(m_pDevice, "Deferred Hybrid Constant Buffer");
    if (!m_pWhiteTexture) m_pWhiteTexture = PipelineUtils::CreateSolidColorTexture(m_pDevice, "Deferred Hybrid No Shadow", 0xFFFFFFFF);
    if (!m_pBlackTexture) m_pBlackTexture = PipelineUtils::CreateSolidColorTexture(m_pDevice, "Deferred Hybrid No Signal", 0x00000000);

    // Formats must match Shaders/DeferredHybrid/gbuffer_common.hlsli. Kept across re-initialization (pipeline switch, MSAA toggle).
    if (!m_GBuffer.IsConfigured()) {
        m_GBuffer.Configure({
                { "Deferred Hybrid G-Buffer AlbedoMetallic", TEX_FORMAT_RGBA8_UNORM_SRGB, "g_GBufferAlbedoMetallic" },
                { "Deferred Hybrid G-Buffer Normals",        TEX_FORMAT_RGBA16_FLOAT,     "g_GBufferNormals" },
                { "Deferred Hybrid G-Buffer Emissive",       TEX_FORMAT_R11G11B10_FLOAT,  "g_GBufferEmissive" },
                { "Deferred Hybrid G-Buffer Material",       TEX_FORMAT_RGBA8_UNORM,      "g_GBufferMaterial" },
                { "Deferred Hybrid G-Buffer Velocity",       TEX_FORMAT_RG16_FLOAT,       "g_GBufferVelocity" },
                { "Deferred Hybrid G-Buffer Sheen",          TEX_FORMAT_RGBA8_UNORM,      "g_GBufferSheen" },
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
    m_DepthPrepass = BuildPass(CreateModelPSOBuilder("Deferred Hybrid Depth Prepass PSO", "DeferredHybrid/depth_prepass_ps.hlsl")
        .RenderTargets({}, m_GBuffer.GetDepthFormat())
        .Depth(true, true, COMPARISON_FUNC_LESS));
}

void Diligent::DeferredHybridPipeline::InitializeGBufferPSO()
{
    // The prepass already wrote the nearest depth: only fragments matching it pass, and nothing is rewritten.
    // LESS_EQUAL instead of EQUAL keeps this robust if the two passes' depths ever differ in the last bit.
    // m_pPSO / m_pSRB so BasePipeline::RenderModels binds materials through this pass.
    BuildMainPass(CreateModelPSOBuilder("Deferred Hybrid G-Buffer PSO", "DeferredHybrid/gbuffer_ps.hlsl")
        .RenderTargets(m_GBuffer.GetColorFormats(), m_GBuffer.GetDepthFormat())
        .Depth(true, false, COMPARISON_FUNC_LESS_EQUAL));
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
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferEmissive", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferMaterial", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferSheen", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        .Variable(SHADER_TYPE_PIXEL, "g_GBufferDepth", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
        // Dynamic: the guide and the signals are other textures from frame to frame
        .Variable(SHADER_TYPE_PIXEL, "g_Guide")
        .Variable(SHADER_TYPE_PIXEL, "g_IndirectSignal")
        .Variable(SHADER_TYPE_PIXEL, "g_ShadowSignal0")
        .Variable(SHADER_TYPE_PIXEL, "g_ShadowSignal1")
        .Variable(SHADER_TYPE_PIXEL, "g_ShadowSignal2")
        .Variable(SHADER_TYPE_PIXEL, "g_ReflectionSignal")
        .Variable(SHADER_TYPE_PIXEL, "g_GlassTint")
        .Variable(SHADER_TYPE_PIXEL, "DeferredHybridConstants")
        .Variable(SHADER_TYPE_PIXEL, "LightConstants")
        .Variable(SHADER_TYPE_PIXEL, "ShadowSettings")
        .Variable(SHADER_TYPE_PIXEL, "FogConstants")
        .Variables(GetRayTracingVariables())
        .Samplers(GetRayTracingSamplers());
    return builder;
}

Diligent::PipelinePass Diligent::DeferredHybridPipeline::BuildSignalPass(PSOBuilder& builder)
{
    PipelinePass pass = BuildPass(builder);
    PipelineUtils::BindVariable(pass.pSRB, SHADER_TYPE_PIXEL, "DeferredHybridConstants", m_pDeferredCB);
    // Re-initialization recreates the SRB, so rebind an existing G-buffer
    m_GBuffer.BindToSRB(pass.pSRB);
    return pass;
}

void Diligent::DeferredHybridPipeline::InitializeSignalPasses()
{
    m_Guide = BuildSignalPass(CreateSignalPassBuilder("Deferred Hybrid Guide PSO", "DeferredHybrid/guide_ps.hlsl")
        .RenderTargets({ kGuideFormat }, TEX_FORMAT_UNKNOWN));

    m_ShadowTrace = BuildSignalPass(CreateSignalPassBuilder("Deferred Hybrid Shadow Trace PSO", "DeferredHybrid/shadow_trace_ps.hlsl")
        .RenderTargets({ kShadowRaysFormat }, TEX_FORMAT_UNKNOWN));

    m_ShadowTraceTint = BuildSignalPass(CreateSignalPassBuilder("Deferred Hybrid Shadow Trace PSO (glass)", "DeferredHybrid/shadow_trace_ps.hlsl")
        .PixelMacros({ { "GLASS_TINT_OUTPUT", "1" } })
        .RenderTargets({ kShadowRaysFormat, kGlassTintFormat }, TEX_FORMAT_UNKNOWN));

    m_AOTrace = BuildSignalPass(CreateSignalPassBuilder("Deferred Hybrid AO Trace PSO", "DeferredHybrid/ao_trace_ps.hlsl")
        .RenderTargets({ kLightRaysFormat }, TEX_FORMAT_UNKNOWN));

    m_ReflectionTrace = BuildSignalPass(CreateSignalPassBuilder("Deferred Hybrid Reflection Trace PSO", "DeferredHybrid/reflection_trace_ps.hlsl")
        .RenderTargets({ kLightRaysFormat }, TEX_FORMAT_UNKNOWN));
}

void Diligent::DeferredHybridPipeline::InitializeLightingPSO()
{
    // The lighting pass writes into the frame's target, which is multisampled when MSAA is on.
    // Shading is still per pixel (the G-buffer is single sampled), so geometry edges are not anti-aliased.
    // The shader outputs the G-buffer depth, so later passes (particles, debug lines) are occluded by the scene.
    // It traces no rays, but lights with the sky cubes, which come with the ray tracing variables.
    m_Lighting = BuildSignalPass(CreateSignalPassBuilder("Deferred Hybrid Lighting PSO", "DeferredHybrid/lighting_ps.hlsl")
        .RenderTargets({ GetColorFormat() }, GetDepthFormat())
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .Depth(true, true, COMPARISON_FUNC_ALWAYS));

    // Draws the glass over the lit scene
    InitializeGlassPSO();

    // Every SRB of GetRayTracingSRBs exists by now
    BindRayTracingResources();
}

void Diligent::DeferredHybridPipeline::RebuildRayPipelines()
{
    // The passes that trace rays: the GI pass, the guide and trace passes, the lighting pass and the glass pass
    InitializeDiffuseGI();
    InitializeSignalPasses();
    InitializeLightingPSO();

    // What was gathered went through the old passes
    m_GuideValid = false;
    ResetSignalHistory();
}

std::vector<Diligent::IShaderResourceBinding*> Diligent::DeferredHybridPipeline::GetRayTracingSRBs()
{
    return { m_Lighting.pSRB, m_GI.GetTraceSRB(), m_ShadowTrace.pSRB, m_ShadowTraceTint.pSRB, m_AOTrace.pSRB, m_ReflectionTrace.pSRB, m_Glass.pSRB };
}

std::vector<Diligent::IShaderResourceBinding*> Diligent::DeferredHybridPipeline::GetGBufferSRBs()
{
    return { m_Lighting.pSRB, m_GI.GetTraceSRB(), m_Guide.pSRB, m_ShadowTrace.pSRB, m_ShadowTraceTint.pSRB, m_AOTrace.pSRB, m_ReflectionTrace.pSRB };
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

void Diligent::DeferredHybridPipeline::UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings)
{
    HybridPipeline::UpdateProjectSettings(pContext, settings);
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
    pContext->SetPipelineState(m_DepthPrepass.pPSO);
    for (const ModelRenderInstance& model : renderData.Models) {
        RenderModelWithSRB(pContext, model, renderOpaque, m_DepthPrepass.pSRB);
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
    m_TintHistory.Reset();
    m_GI.ResetHistory();
}

void Diligent::DeferredHybridPipeline::PrepareSignalTargets(Uint32 width, Uint32 height)
{
    if (width == m_SignalWidth && height == m_SignalHeight && m_pGuide[0] && m_pGuide[1] && m_pReflectionRays && m_pGlassTintRays) return;

    m_SignalWidth = width;
    m_SignalHeight = height;

    m_pGuide[0] = CreateRenderTarget(m_pDevice, "Deferred Hybrid Guide 0", kGuideFormat, width, height);
    m_pGuide[1] = CreateRenderTarget(m_pDevice, "Deferred Hybrid Guide 1", kGuideFormat, width, height);
    m_pReflectionRays = CreateRenderTarget(m_pDevice, "Deferred Hybrid Reflection Rays", kLightRaysFormat, width, height);
    m_pGlassTintRays = CreateRenderTarget(m_pDevice, "Deferred Hybrid Glass Tint Rays", kGlassTintFormat, width, height);
    // Created again when a frame needs them
    for (RefCntAutoPtr<ITexture>& pShadowRays : m_pShadowRays) {
        pShadowRays.Release();
    }
    m_pAORays.Release();

    // Nothing gathered at the old size can be reused
    m_GuideValid = false;
    ResetSignalHistory();
}

void Diligent::DeferredHybridPipeline::DrawSignalPass(IDeviceContext* pContext, const PipelinePass& pass, std::initializer_list<ITexture*> targets)
{
    ITextureView* pRTVs[kMaxSignalTargets] = {};
    Uint32 count = 0;
    for (ITexture* pTarget : targets) {
        if (count < kMaxSignalTargets) pRTVs[count++] = RTV(pTarget);
    }

    // The targets are as large as the pass's output, so the viewport binding them sets is the right one
    pContext->SetRenderTargets(count, pRTVs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    PipelineUtils::DrawFullscreenPass(pContext, pass.pPSO, pass.pSRB);
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
    if (!m_pGuide[0] || !m_pGuide[1] || !m_pReflectionRays || !m_pGlassTintRays || !m_Guide || !m_ShadowTrace || !m_ShadowTraceTint || !m_AOTrace
        || !m_ReflectionTrace || !m_Lighting) return;

    SignalFrame frame;
    frame.Width = width;
    frame.Height = height;

    // 3. Guide
    BeginSignalFrame(pContext, frame);
    // 4. The trace passes, each followed by its denoising
    TraceShadows(pContext, frame);
    TraceIndirect(pContext, frame);
    TraceReflections(pContext, frame);
    // 5. Lighting
    ComposeLighting(pContext, target, frame);

    // 6. Glass, over the lit scene. The depth the lighting pass wrote is what it is tested against.
    RenderGlass(pContext, target);

    // This frame becomes "last frame"
    m_GuideCurrent = frame.CurrentGuide;
    m_GuideValid = true;
    m_PrevCameraPos = m_CameraPos;
}

void Diligent::DeferredHybridPipeline::BeginSignalFrame(IDeviceContext* pContext, SignalFrame& frame)
{
    const Uint32 width = frame.Width;
    const Uint32 height = frame.Height;

    // Which signals go through the denoiser (the project's Denoising settings). The others are shown as traced.
    const bool denoise = m_DenoiseSettings.enabled && m_Denoiser.IsInitialized();
    frame.DenoiseShadows = denoise && m_DenoiseSettings.shadows;
    frame.DenoiseIndirect = denoise && m_DenoiseSettings.indirect;
    frame.DenoiseReflections = denoise && m_DenoiseSettings.reflections;

    // Light that crosses glass takes its color. That needs a signal of its own, written by the first shadow layer's
    // pass, so it covers the first four lights (the others still lose the glass's brightness)
    frame.TintShadows = m_TransparencySettings.enabled && m_TransparencySettings.glass && m_TransparencySettings.tinted_shadows
        && m_SceneTLAS.HasGlass() && (m_NumDirLights + m_NumPointLights) > 0;

    // Per-pixel GI, where the project asks for it and the device can run it; any other GI method is Sky + AO
    frame.PerPixelGI = m_GISettings.method == GI_METHOD_PER_PIXEL && m_GI.IsInitialized();
    // View pixels per pixel of the indirect signal, along each axis: each method has its own half resolution switch
    frame.IndirectScale = frame.PerPixelGI ? DiffuseGIPass::GetScale(m_GISettings) : (m_GISettings.ao_half_resolution ? 2u : 1u);

    DeferredHybridConstants& constants = frame.Constants;
    // Transposed like CameraConstants, so HLSL's mul(clipPos, g_InvViewProj) undoes mul(mul(worldPos, g_View), g_Proj)
    constants.InvViewProj = glm::transpose(glm::inverse(m_ViewProj));
    constants.ViewportSize = glm::vec4(static_cast<float>(width), static_cast<float>(height), 1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height));
    constants.ViewParams = glm::vec4(m_PixelAngle, 0.0f, 0.0f, 0.0f);
    constants.ViewOrigin = glm::vec4(m_CameraPos, 1.0f);
    constants.GIParams = glm::vec4(frame.PerPixelGI ? 1.0f : 0.0f, static_cast<float>(frame.IndirectScale), (std::max)(m_GISettings.intensity, 0.0f), 0.0f);
    constants.SignalFrame = ++m_SignalFrame;
    if (frame.TintShadows) constants.GlassFlags |= GLASS_TINT_SHADOWS;
    if (frame.DenoiseShadows) constants.DenoiseFlags |= DENOISE_SHADOWS;
    if (frame.DenoiseIndirect) constants.DenoiseFlags |= DENOISE_INDIRECT;
    if (frame.DenoiseReflections) constants.DenoiseFlags |= DENOISE_REFLECTIONS;
    PipelineUtils::WriteCB(pContext, m_pDeferredCB, constants);

    // The guide. Last frame's stays in the other texture of the pair.
    const Uint32 previousGuide = m_GuideCurrent;
    frame.CurrentGuide = 1 - m_GuideCurrent;
    DrawSignalPass(pContext, m_Guide, { m_pGuide[frame.CurrentGuide] });
    frame.pGuide = SRV(m_pGuide[frame.CurrentGuide]);
    frame.pPrevGuide = m_GuideValid ? SRV(m_pGuide[previousGuide]) : nullptr;

    SignalDenoiser::Frame& denoiseFrame = frame.Denoise;
    denoiseFrame.ViewProj = m_ViewProj;
    denoiseFrame.ViewProjNoJitter = m_ViewProjNoJitter;
    denoiseFrame.PrevViewProjNoJitter = m_PrevViewProjNoJitter;
    denoiseFrame.CameraPos = m_CameraPos;
    denoiseFrame.PrevCameraPos = m_PrevCameraPos;
    denoiseFrame.Width = width;
    denoiseFrame.Height = height;
    denoiseFrame.PixelAngle = m_PixelAngle;
    denoiseFrame.pGuide = frame.pGuide;
    denoiseFrame.pPrevGuide = frame.pPrevGuide;
    denoiseFrame.pVelocity = GetVelocitySRV();
    denoiseFrame.HistoryValid = m_GuideValid;
}

void Diligent::DeferredHybridPipeline::TraceShadows(IDeviceContext* pContext, SignalFrame& frame)
{
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
    for (Uint32 layer = 0; layer < kMaxShadowLayers; ++layer) {
        frame.pShadows[layer] = SRV(m_pWhiteTexture);

        if (layer < shadowLayers && !m_pShadowRays[layer]) {
            m_pShadowRays[layer] = CreateRenderTarget(m_pDevice, "Deferred Hybrid Shadow Rays", kShadowRaysFormat, frame.Width, frame.Height);
        }
        if (layer >= shadowLayers || !m_pShadowRays[layer]) {
            m_ShadowHistory[layer].Reset();
            continue;
        }

        frame.Constants.ShadowLayer = layer;
        PipelineUtils::WriteCB(pContext, m_pDeferredCB, frame.Constants);
        if (frame.TintShadows && layer == 0) {
            DrawSignalPass(pContext, m_ShadowTraceTint, { m_pShadowRays[layer], m_pGlassTintRays });
        }
        else {
            DrawSignalPass(pContext, m_ShadowTrace, { m_pShadowRays[layer] });
        }

        frame.pShadows[layer] = SRV(m_pShadowRays[layer]);
        if (frame.DenoiseShadows) {
            frame.pShadows[layer] = m_Denoiser.Denoise(pContext, frame.Denoise, m_ShadowHistory[layer], frame.pShadows[layer], 1, shadowSettings);
        }
        else {
            m_ShadowHistory[layer].Reset();
        }
    }

    // The glass tint, from the pass that traced layer 0's shadows
    frame.pGlassTint = SRV(m_pWhiteTexture);
    if (frame.TintShadows) {
        frame.pGlassTint = SRV(m_pGlassTintRays);
        if (frame.DenoiseShadows) {
            SignalDenoiser::Settings tintSettings;
            tintSettings.SignalKind = SignalDenoiser::SIGNAL_LIGHT;
            tintSettings.MinBlend = kGlassTintMinBlend;
            tintSettings.FilterPasses = kGlassTintFilterPasses;
            tintSettings.ClampSigma = kShadowClampSigma;
            tintSettings.BrightnessSigma = kShadowBrightnessSigma;
            frame.pGlassTint = m_Denoiser.Denoise(pContext, frame.Denoise, m_TintHistory, frame.pGlassTint, 1, tintSettings);
        }
        else {
            m_TintHistory.Reset();
        }
    }
    else {
        m_TintHistory.Reset();
    }
}

void Diligent::DeferredHybridPipeline::TraceIndirect(IDeviceContext* pContext, SignalFrame& frame)
{
    // 4b. Indirect light: per-pixel GI rays, or one ambient occlusion ray for Sky + AO. The two fill the same
    //     texture with different things, so a history gathered for one is no use to the other.
    const bool perPixelGI = frame.PerPixelGI;
    const Uint32 indirectScale = frame.IndirectScale;
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
        view.Width = frame.Width;
        view.Height = frame.Height;
        view.pPrevGuide = frame.pPrevGuide;
        // The GI that was shown last frame, for light that bounces on. Without a denoiser the pass reads its own rays.
        view.pPrevResult = frame.DenoiseIndirect ? m_IndirectHistory.GetResultSRV() : nullptr;
        view.Denoised = frame.DenoiseIndirect;
        view.NoiseFrame = m_NoiseFrame;
        pIndirect = m_GI.Trace(pContext, view, m_GISettings);

        indirectSettings.MinBlend = 1.0f - glm::clamp(m_GISettings.history_blend, 0.0f, 0.99f);
        indirectSettings.FilterPasses = m_GISettings.filter_passes;
        indirectSettings.BrightnessSigma = kGIBrightnessSigma;
        indirectSettings.PreserveLight = m_GISettings.preserve_light;
    }
    else {
        // Whatever the GI pass had is stale by the time it is switched back on
        m_GI.ResetHistory();

        // The rays' texture is as large as the signal, so it is made again when the half resolution switch changes
        const Uint32 aoWidth = (frame.Width + indirectScale - 1) / indirectScale;
        const Uint32 aoHeight = (frame.Height + indirectScale - 1) / indirectScale;
        if (!m_pAORays || m_pAORays->GetDesc().Width != aoWidth || m_pAORays->GetDesc().Height != aoHeight) {
            m_pAORays = CreateRenderTarget(m_pDevice, "Deferred Hybrid AO Rays", kLightRaysFormat, aoWidth, aoHeight);
        }
        if (m_pAORays) {
            DrawSignalPass(pContext, m_AOTrace, { m_pAORays });
            pIndirect = SRV(m_pAORays);
        }

        indirectSettings.MinBlend = kAOMinBlend;
        indirectSettings.FilterPasses = indirectScale > 1 ? kAOHalfResolutionFilterPasses : kAOFilterPasses;
    }

    if (frame.DenoiseIndirect && pIndirect) {
        pIndirect = m_Denoiser.Denoise(pContext, frame.Denoise, m_IndirectHistory, pIndirect, indirectScale, indirectSettings);
    }
    else {
        m_IndirectHistory.Reset();
    }
    if (!pIndirect) pIndirect = SRV(m_pBlackTexture);
    frame.pIndirect = pIndirect;
}

void Diligent::DeferredHybridPipeline::TraceReflections(IDeviceContext* pContext, SignalFrame& frame)
{
    // 4c. Reflections: one ray per pixel smooth enough to get one. What a ray hits is lit by the indirect light
    //     gathered at its pixel, so this comes after 4b. Skipped when the lighting pass wouldn't read the result.
    frame.pReflection = SRV(m_pBlackTexture);
    if (m_ReflectionSettings.enabled && m_ReflectionIntensity > 0.0f) {
        PipelineUtils::BindVariable(m_ReflectionTrace.pSRB, SHADER_TYPE_PIXEL, "g_Guide", frame.pGuide);
        PipelineUtils::BindVariable(m_ReflectionTrace.pSRB, SHADER_TYPE_PIXEL, "g_IndirectSignal", frame.pIndirect);
        DrawSignalPass(pContext, m_ReflectionTrace, { m_pReflectionRays });
        frame.pReflection = SRV(m_pReflectionRays);

        if (frame.DenoiseReflections) {
            SignalDenoiser::Settings reflectionSettings;
            reflectionSettings.SignalKind = SignalDenoiser::SIGNAL_REFLECTION;
            reflectionSettings.MinBlend = kReflectionMinBlend;
            reflectionSettings.FilterPasses = kReflectionFilterPasses;
            reflectionSettings.ClampSigma = kReflectionClampSigma;
            reflectionSettings.BrightnessSigma = kReflectionBrightnessSigma;
            // The limit the shaders trace up to, see BuildShadowSettings
            reflectionSettings.MaxRoughness = glm::clamp(m_ReflectionSettings.max_roughness, 0.01f, 1.0f);
            reflectionSettings.VirtualMotion = true;
            frame.pReflection = m_Denoiser.Denoise(pContext, frame.Denoise, m_ReflectionHistory, frame.pReflection, 1, reflectionSettings);
        }
        else {
            m_ReflectionHistory.Reset();
        }
    }
    else {
        m_ReflectionHistory.Reset();
    }
}

void Diligent::DeferredHybridPipeline::ComposeLighting(IDeviceContext* pContext, const FrameTarget& target, const SignalFrame& frame)
{
    // 5. Lighting: composes each visible pixel from the G-buffer and the signals, over the skybox already in the target
    IShaderResourceBinding* pSRB = m_Lighting.pSRB;
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_Guide", frame.pGuide);
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_IndirectSignal", frame.pIndirect);
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_ShadowSignal0", frame.pShadows[0]);
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_ShadowSignal1", frame.pShadows[1]);
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_ShadowSignal2", frame.pShadows[2]);
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_ReflectionSignal", frame.pReflection);
    PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "g_GlassTint", frame.pGlassTint);

    target.Bind(pContext);
    PipelineUtils::DrawFullscreenPass(pContext, m_Lighting.pPSO, pSRB);
}
