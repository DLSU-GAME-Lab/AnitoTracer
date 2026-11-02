#include "RendererManager.hpp"
#include "RendererManager.hpp"
#include "RendererManager.hpp"
#include "../Asset/ProjectLoader.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <type_traits>

namespace {
    // Converts the project's fog settings into the layout expected by Shaders/fog.hlsli
    Diligent::FogConstants BuildFogConstants(const ProjectFogSettings& settings)
    {
        static const auto startTime = std::chrono::steady_clock::now();
        const float elapsedSeconds = std::chrono::duration<float>(std::chrono::steady_clock::now() - startTime).count();

        Diligent::FogConstants fog;
        fog.LinearColor = glm::vec4(settings.linear_color[0], settings.linear_color[1], settings.linear_color[2], settings.linear_max_opacity);
        fog.VolumetricColor = glm::vec4(settings.volumetric_color[0], settings.volumetric_color[1], settings.volumetric_color[2], settings.volumetric_density);
        fog.LinearStart = settings.linear_start;
        fog.LinearEnd = settings.linear_end;
        fog.HeightFalloff = settings.volumetric_height_falloff;
        fog.BaseHeight = settings.volumetric_base_height;
        fog.Anisotropy = glm::clamp(settings.volumetric_anisotropy, -0.95f, 0.95f);
        fog.MaxDistance = settings.volumetric_max_distance;
        fog.NoiseStrength = settings.volumetric_noise_strength;
        fog.NoiseScale = settings.volumetric_noise_scale;
        fog.Wind = glm::vec4(settings.volumetric_wind[0], settings.volumetric_wind[1], settings.volumetric_wind[2], elapsedSeconds);
        fog.LinearEnabled = settings.linear_enabled ? 1 : 0;
        fog.VolumetricEnabled = settings.volumetric_enabled ? 1 : 0;
        fog.ApplyToSkybox = settings.apply_to_skybox ? 1 : 0;
        fog.SampleCount = glm::clamp(settings.volumetric_sample_count, 1, 128);
        fog.LightIntensity = settings.volumetric_light_intensity;
        fog.AmbientIntensity = settings.volumetric_ambient_intensity;
        return fog;
    }

    // Scales a tint so it shifts hue without changing brightness; black/invalid tints fall back to neutral
    glm::vec3 NormalizeTint(const std::array<float, 3>& tint)
    {
        const glm::vec3 color(tint[0], tint[1], tint[2]);
        const float luminance = glm::dot(color, glm::vec3(0.2126f, 0.7152f, 0.0722f));
        return luminance > 1e-3f ? color / luminance : glm::vec3(1.0f);
    }

    // Converts the project's color correction settings into the layout expected by Shaders/PostProcess/color_correction_ps.hlsl
    Diligent::ColorCorrectionConstants BuildColorCorrectionConstants(const ProjectColorCorrectionSettings& settings)
    {
        const float temperature = glm::clamp(settings.temperature, -1.0f, 1.0f);
        const float tint = glm::clamp(settings.tint, -1.0f, 1.0f);
        const glm::vec3 whiteBalance = NormalizeTint({ 1.0f + 0.3f * temperature, 1.0f - 0.3f * tint, 1.0f - 0.3f * temperature });
        const glm::vec3 filter(settings.color_filter[0], settings.color_filter[1], settings.color_filter[2]);

        Diligent::ColorCorrectionConstants cc;
        cc.ColorFilter = glm::vec4(whiteBalance * filter, std::exp2(settings.exposure));
        cc.Shadows = glm::vec4(NormalizeTint(settings.shadows), 1.0f);
        cc.Midtones = glm::vec4(NormalizeTint(settings.midtones), 1.0f);
        cc.Highlights = glm::vec4(NormalizeTint(settings.highlights), 1.0f);
        cc.Contrast = glm::max(settings.contrast, 0.0f);
        cc.Saturation = glm::max(settings.saturation, 0.0f);
        cc.InvGamma = 1.0f / glm::max(settings.gamma, 0.01f);
        cc.HueShift = glm::radians(glm::clamp(settings.hue_shift, -180.0f, 180.0f));
        return cc;
    }

    // Converts the project's god rays settings into the layout expected by Shaders/PostProcess/god_rays_ps.hlsl.
    // LightColor.a comes back 0 when there is nothing to draw (no directional light, or the sun is behind the camera).
    Diligent::GodRaysConstants BuildGodRaysConstants(const ProjectGodRaysSettings& settings, const Diligent::RenderData& renderData, Diligent::Uint32 width, Diligent::Uint32 height)
    {
        Diligent::GodRaysConstants gr;
        if (renderData.Lights.NumDirLights <= 0 || width == 0 || height == 0) return gr;

        const auto& sun = renderData.Lights.DirLights[0];
        const glm::vec3 toSun = -glm::normalize(glm::vec3(sun.Direction));

        // w = 0 projects the direction itself, i.e. a point infinitely far away
        const glm::vec4 viewDir = renderData.ViewMatrix * glm::vec4(toSun, 0.0f);
        const glm::vec4 clip = renderData.ProjectionMatrix * viewDir;
        if (clip.w <= 1e-4f) return gr;

        const glm::vec2 ndc = glm::vec2(clip) / clip.w;
        gr.SunUV = glm::vec2(ndc.x * 0.5f + 0.5f, 0.5f - ndc.y * 0.5f);

        // Fade out as the sun approaches the camera's side plane or drifts far off-screen
        const float facingFade = glm::smoothstep(0.0f, 0.25f, glm::normalize(glm::vec3(viewDir)).z);
        const float edgeFade = 1.0f - glm::smoothstep(1.0f, 2.0f, glm::max(glm::abs(ndc.x), glm::abs(ndc.y)));

        glm::vec3 color(settings.tint[0], settings.tint[1], settings.tint[2]);
        if (settings.use_light_color) color *= glm::vec3(sun.Color) * sun.Color.a;

        const int sampleCount = glm::clamp(settings.sample_count, 8, 128);
        const float decay = std::pow(glm::clamp(settings.decay, 0.5f, 0.999f), 64.0f / sampleCount);

        gr.LightColor = glm::vec4(color, glm::max(settings.intensity, 0.0f) * facingFade * edgeFade);
        gr.Density = glm::clamp(settings.density, 0.0f, 1.0f);
        gr.Decay = decay;
        // Normalizes the decaying sum of all samples to 1, so decay and sample count only change the shape, not the brightness
        gr.Weight = (1.0f - decay) / (1.0f - std::pow(decay, static_cast<float>(sampleCount)));
        gr.Threshold = glm::clamp(settings.threshold, 0.0f, 0.99f);
        gr.SunRadius = glm::max(settings.sun_radius, 0.01f);
        gr.AspectRatio = static_cast<float>(width) / static_cast<float>(height);
        gr.SampleCount = sampleCount;
        return gr;
    }
}

void RendererManager::Initialize(Diligent::IRenderDevice* pDevice, Diligent::IDeviceContext* pContext, Diligent::ISwapChain* pSwapChain, bool supportsRayTracing)
{
    m_pDevice = pDevice;
    m_pImmediateContext = pContext;
    m_pSwapChain = pSwapChain;
    m_SupportsRayTracing = supportsRayTracing;

    auto& userSettings = Diligent::UserSettings::GetInstance();

    // Initialize pipeline based on the saved user setting
    if (userSettings.GetRendererType() == Diligent::PipelineType::HYBRID && supportsRayTracing)
    {
        m_bLitPipeline.emplace<Diligent::HybridPipeline>();
        std::cout << "[Info] Hardware Ray Tracing detected. Using HybridPipeline." << std::endl;
    }
    else if (userSettings.GetRendererType() == Diligent::PipelineType::DEFERRED)
    {
        m_bLitPipeline.emplace<Diligent::DeferredPipeline>();
        std::cout << "[Info] Using DeferredPipeline." << std::endl;
    }
    else
    {
        m_bLitPipeline.emplace<Diligent::BasicLitPipeline>();
        userSettings.GetRendererType() = Diligent::PipelineType::BASIC_LIT; // Force valid fallback

        if (userSettings.GetRendererType() == Diligent::PipelineType::HYBRID && !supportsRayTracing)
        {
            std::cout << "[Warn] Hardware Ray Tracing not available. Falling back to BasicLitPipeline." << std::endl;
        }
        else
        {
            std::cout << "[Info] Using BasicLitPipeline." << std::endl;
        }
    }

    // m_DebugLinePipeline.InitializePipeline(pDevice, pSwapChain);

    m_LastMSAAState = userSettings.GetEnableMSAA();
    CreateMSAABuffers();

    m_OnRendererChangeSub = gbe::ScopedSubscription::Create<RendererChangeArgs>(
        EVENT_RENDER_CHANGE,
        &RendererManager::HandleRendererChangeEvent,
        this
    );
}

void RendererManager::HandleRendererChangeEvent(const RendererChangeArgs* args)
{
    auto& userSettings = Diligent::UserSettings::GetInstance();
    bool isMSAAEnabled = userSettings.GetEnableMSAA();

    if (args->targetPipeline == Diligent::PipelineType::DEFERRED)
    {
        if (isMSAAEnabled)
        {
            std::cout << "[Warn] Deferred rendering does not support MSAA with current setup. Reverting to Hybrid/Basic Lit." << std::endl;
            if (m_SupportsRayTracing)
            {
                m_bLitPipeline.emplace<Diligent::HybridPipeline>();
                userSettings.GetRendererType() = Diligent::PipelineType::HYBRID;
            }
            else
            {
                m_bLitPipeline.emplace<Diligent::BasicLitPipeline>();
                userSettings.GetRendererType() = Diligent::PipelineType::BASIC_LIT;
            }
        }
        else
        {
            m_bLitPipeline.emplace<Diligent::DeferredPipeline>();
            userSettings.GetRendererType() = Diligent::PipelineType::DEFERRED;
            std::cout << "[RendererManager] Swapped to Deferred Pipeline." << std::endl;
        }
    }
    else if (args->targetPipeline == Diligent::PipelineType::HYBRID && m_SupportsRayTracing)
    {
        m_bLitPipeline.emplace<Diligent::HybridPipeline>();
        userSettings.GetRendererType() = Diligent::PipelineType::HYBRID;
        std::cout << "[RendererManager] Swapped to Hybrid Pipeline." << std::endl;
    }
    else
    {
        m_bLitPipeline.emplace<Diligent::BasicLitPipeline>();
        userSettings.GetRendererType() = Diligent::PipelineType::BASIC_LIT;
        std::cout << "[RendererManager] Swapped to Basic Lit Pipeline." << std::endl;
    }

    InitializePipelines();
}

void RendererManager::InitializePipelines()
{
    const auto& SCDesc = m_pSwapChain->GetDesc();

    std::visit([&](auto& pipeline) {
        pipeline.InitializePipeline(m_pDevice, m_pSwapChain);
        }, m_bLitPipeline);

    m_DebugLinePipeline.InitializePipeline(m_pDevice, m_pSwapChain);
    
    TEXTURE_FORMAT RTVFormat = SCDesc.ColorBufferFormat;
    TEXTURE_FORMAT DSVFormat = SCDesc.DepthBufferFormat;
    m_SkyboxPipeline.Initialize(m_pDevice, RTVFormat, DSVFormat);
    m_GodRaysPipeline.Initialize(m_pDevice, RTVFormat);
    m_ColorCorrectionPipeline.Initialize(m_pDevice, RTVFormat);

    // Load your default skybox texture (Make sure it is a compiled .dds cubemap!)
    // FOR TESTING
    auto* pDefaultSkybox = ModelManager::GetInstance().LoadCubemap("Assets/Skyboxes/skybox_raw.dds", true);
    if (pDefaultSkybox) {
        m_SkyboxPipeline.SetCubemap(pDefaultSkybox);
        if (auto* pHybrid = std::get_if<Diligent::HybridPipeline>(&m_bLitPipeline)) {
            pHybrid->SetEnvironmentMap(pDefaultSkybox);
        }
        std::cout << "[Info] Default Skybox loaded and assigned successfully." << std::endl;
    }
    else {
        std::cout << "[Warn] Could not load Default Skybox." << std::endl;
    }
}

void RendererManager::CreateMSAABuffers()
{
    if (!m_pSwapChain) return;

    const auto& SCDesc = m_pSwapChain->GetDesc();
    Diligent::Uint8 sampleCount = Diligent::UserSettings::GetInstance().GetEnableMSAA() ? 4 : 1;

    Diligent::TextureDesc ColorDesc;
    ColorDesc.Name = "MSAA Color Target";
    ColorDesc.Type = Diligent::RESOURCE_DIM_TEX_2D;
    ColorDesc.Width = SCDesc.Width;
    ColorDesc.Height = SCDesc.Height;
    ColorDesc.BindFlags = Diligent::BIND_RENDER_TARGET;
    ColorDesc.Format = SCDesc.ColorBufferFormat;
    ColorDesc.SampleCount = sampleCount;

    m_pMSAATarget.Release();
    m_pDevice->CreateTexture(ColorDesc, nullptr, &m_pMSAATarget);
    m_pMSAARTV = m_pMSAATarget->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);

    Diligent::TextureDesc DepthDesc = ColorDesc;
    DepthDesc.Name = "MSAA Depth Buffer";
    DepthDesc.BindFlags = Diligent::BIND_DEPTH_STENCIL;
    DepthDesc.Format = SCDesc.DepthBufferFormat;

    m_pMSAADepth.Release();
    m_pDevice->CreateTexture(DepthDesc, nullptr, &m_pMSAADepth);
    m_pMSAADSV = m_pMSAADepth->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
}

void RendererManager::OnResize(Diligent::Uint32 width, Diligent::Uint32 height)
{
    if (m_pSwapChain)
    {
        // Let the swap chain handle the raw dimensions
        m_pSwapChain->Resize(width, height);

        // Fetch the actual resolved dimensions
        const auto& SCDesc = m_pSwapChain->GetDesc();

        // Prevent creating 0-sized G-Buffers when minimized or transitioning
        if (SCDesc.Width == 0 || SCDesc.Height == 0) return;

        if (auto* pDeferred = std::get_if<Diligent::DeferredPipeline>(&m_bLitPipeline))
        {
            // Use the safe SwapChain descriptor dimensions
            pDeferred->OnWindowResize(m_pDevice, SCDesc.Width, SCDesc.Height);
        }
    }
}

void RendererManager::RenderFrame(const Diligent::RenderData& renderData)
{
    const auto& SCDesc = m_pSwapChain->GetDesc();
    if (SCDesc.Width == 0 || SCDesc.Height == 0) return;

    bool isMSAAEnabled = UserSettings::GetInstance().GetEnableMSAA();

    if (m_pMSAATarget->GetDesc().Width != SCDesc.Width ||
        m_pMSAATarget->GetDesc().Height != SCDesc.Height ||
        m_LastMSAAState != isMSAAEnabled)
    {
        CreateMSAABuffers();

        if (m_LastMSAAState != isMSAAEnabled)
        {
            //TODO Fix this for deffered later
            // If deferred is active and MSAA was just turned ON, force a switch to a compatible pipeline
            if (isMSAAEnabled && std::holds_alternative<Diligent::DeferredPipeline>(m_bLitPipeline))
            {
                if (m_SupportsRayTracing)
                    m_bLitPipeline.emplace<Diligent::HybridPipeline>();
                else
                    m_bLitPipeline.emplace<Diligent::BasicLitPipeline>();
            }

            InitializePipelines();
            m_LastMSAAState = isMSAAEnabled;
        }
    }

    // The release build is the game itself, so post processing always applies here.
    // Each enabled pass renders into its own intermediate texture: scene -> god rays -> color correction -> back buffer.
    const auto& colorCorrection = ProjectLoader::GetProjectSettings().color_correction;
    Diligent::ITextureView* pGradeRTV = colorCorrection.enabled ? m_ColorCorrectionPipeline.GetIntermediateRTV(SCDesc.Width, SCDesc.Height) : nullptr;
    Diligent::ITextureView* pPostRTV = pGradeRTV ? pGradeRTV : m_pSwapChain->GetCurrentBackBufferRTV();

    const auto& godRays = ProjectLoader::GetProjectSettings().god_rays;
    const Diligent::GodRaysConstants godRaysConstants = BuildGodRaysConstants(godRays, renderData, SCDesc.Width, SCDesc.Height);
    Diligent::ITextureView* pShaftsRTV = (godRays.enabled && godRaysConstants.LightColor.a > 0.0f)
        ? m_GodRaysPipeline.GetIntermediateRTV(SCDesc.Width, SCDesc.Height)
        : nullptr;
    Diligent::ITextureView* pSceneRTV = pShaftsRTV ? pShaftsRTV : pPostRTV;

    const float clearColor[] = { 0.1f, 0.15f, 0.25f, 1.0f };
    Diligent::ITextureView* pActiveRTV = isMSAAEnabled ? m_pMSAARTV : pSceneRTV;
    Diligent::ITextureView* pActiveDSV = isMSAAEnabled ? m_pMSAADSV : m_pSwapChain->GetDepthBufferDSV();

    m_pImmediateContext->SetRenderTargets(1, &pActiveRTV, pActiveDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearRenderTarget(pActiveRTV, clearColor, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pActiveDSV, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const Diligent::FogConstants fogConstants = BuildFogConstants(ProjectLoader::GetProjectSettings().fog);
    m_SkyboxPipeline.UpdateFogConstants(m_pImmediateContext, fogConstants);
    m_SkyboxPipeline.Render(m_pImmediateContext, renderData);

    std::visit([&](auto& pipeline) {
        pipeline.StartFrameRender(m_pImmediateContext, renderData);
        pipeline.UpdateLights(m_pImmediateContext, renderData.Lights);
        pipeline.UpdateShadowSettings(m_pImmediateContext, ProjectLoader::GetProjectSettings().lighting);
        pipeline.UpdateFogConstants(m_pImmediateContext, fogConstants);
        pipeline.RenderModels(m_pImmediateContext, renderData, true);

        using T = std::decay_t<decltype(pipeline)>;
        if constexpr (std::is_same_v<T, Diligent::DeferredPipeline>)
        {
            pipeline.RenderLightingPass(m_pImmediateContext, pActiveRTV);
        }
        }, m_bLitPipeline);

    auto* pBackBufferRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDefaultDSV = m_pSwapChain->GetDepthBufferDSV();

    if (!renderData.DebugLines.empty()) {
        m_pImmediateContext->SetRenderTargets(1, &pActiveRTV, pActiveDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        m_DebugLinePipeline.StartFrameRender(m_pImmediateContext, renderData);
        m_DebugLinePipeline.RenderLines(m_pImmediateContext, renderData.DebugLines);
    }

    if (isMSAAEnabled)
    {
        Diligent::ResolveTextureSubresourceAttribs ResolveAttribs;
        ResolveAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        ResolveAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

        m_pImmediateContext->ResolveTextureSubresource(
            m_pMSAATarget,
            pSceneRTV->GetTexture(),
            ResolveAttribs
        );
    }

    if (pShaftsRTV)
    {
        m_GodRaysPipeline.Apply(m_pImmediateContext, pPostRTV, godRaysConstants);
    }

    if (pGradeRTV)
    {
        m_ColorCorrectionPipeline.Apply(m_pImmediateContext, pBackBufferRTV, BuildColorCorrectionConstants(colorCorrection));
    }

    m_pImmediateContext->SetRenderTargets(1, &pBackBufferRTV, pDefaultDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void RendererManager::Shutdown()
{
    m_pMSAARTV.Release();
    m_pMSAADSV.Release();
    m_pMSAATarget.Release();
    m_pMSAADepth.Release();
    m_pSwapChain.Release();
    m_pImmediateContext.Release();
    m_pDevice.Release();
}

Diligent::RenderTarget* RendererManager::CreateRenderTarget(const std::string& name)
{
    auto rt = std::make_unique<Diligent::RenderTarget>(name);
    Diligent::RenderTarget* ptr = rt.get();
    m_RenderTargets.push_back(std::move(rt));
    return ptr;
}

void RendererManager::RenderToTarget(Diligent::RenderTarget* pTarget, const Diligent::RenderData& renderData, bool applyPostProcessing)
{
    if (!pTarget || !pTarget->GetRTV() || !pTarget->GetDSV()) return;

    bool isMSAAEnabled = UserSettings::GetInstance().GetEnableMSAA();

    // Each enabled post pass renders into its own intermediate texture: scene -> god rays -> color correction -> target
    const auto& colorCorrection = ProjectLoader::GetProjectSettings().color_correction;
    Diligent::ITextureView* pGradeRTV = (applyPostProcessing && colorCorrection.enabled)
        ? m_ColorCorrectionPipeline.GetIntermediateRTV(pTarget->GetWidth(), pTarget->GetHeight())
        : nullptr;
    Diligent::ITextureView* pPostRTV = pGradeRTV ? pGradeRTV : pTarget->GetRTV();

    const auto& godRays = ProjectLoader::GetProjectSettings().god_rays;
    const Diligent::GodRaysConstants godRaysConstants = BuildGodRaysConstants(godRays, renderData, pTarget->GetWidth(), pTarget->GetHeight());
    Diligent::ITextureView* pShaftsRTV = (applyPostProcessing && godRays.enabled && godRaysConstants.LightColor.a > 0.0f)
        ? m_GodRaysPipeline.GetIntermediateRTV(pTarget->GetWidth(), pTarget->GetHeight())
        : nullptr;
    Diligent::ITextureView* pSceneRTV = pShaftsRTV ? pShaftsRTV : pPostRTV;

    // 1. Route to MSAA buffers if enabled, to match the Pipeline's expected SampleCount
    Diligent::ITextureView* pActiveRTV = isMSAAEnabled ? m_pMSAARTV : pSceneRTV;
    Diligent::ITextureView* pActiveDSV = isMSAAEnabled ? m_pMSAADSV : pTarget->GetDSV();

    const float clearColor[] = { 0.1f, 0.15f, 0.25f, 1.0f };
    m_pImmediateContext->SetRenderTargets(1, &pActiveRTV, pActiveDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearRenderTarget(pActiveRTV, clearColor, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pActiveDSV, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    // 2. EXPLICITLY set the Viewport! Without this, Vulkan won't know where to draw.
    Diligent::Viewport Viewport;
    Viewport.Width = static_cast<float>(pTarget->GetWidth());
    Viewport.Height = static_cast<float>(pTarget->GetHeight());
    Viewport.TopLeftX = 0;
    Viewport.TopLeftY = 0;
    Viewport.MinDepth = 0.0f;
    Viewport.MaxDepth = 1.0f;
    m_pImmediateContext->SetViewports(1, &Viewport, pTarget->GetWidth(), pTarget->GetHeight());

    const Diligent::FogConstants fogConstants = BuildFogConstants(ProjectLoader::GetProjectSettings().fog);
    m_SkyboxPipeline.UpdateFogConstants(m_pImmediateContext, fogConstants);
    m_SkyboxPipeline.Render(m_pImmediateContext, renderData);

    std::visit([&](auto& pipeline) {
        pipeline.StartFrameRender(m_pImmediateContext, renderData);
        pipeline.UpdateLights(m_pImmediateContext, renderData.Lights);
        pipeline.UpdateShadowSettings(m_pImmediateContext, ProjectLoader::GetProjectSettings().lighting);
        pipeline.UpdateFogConstants(m_pImmediateContext, fogConstants);
        pipeline.RenderModels(m_pImmediateContext, renderData, true);

        using T = std::decay_t<decltype(pipeline)>;
        if constexpr (std::is_same_v<T, Diligent::DeferredPipeline>) {
            pipeline.RenderLightingPass(m_pImmediateContext, pActiveRTV);
        }
        }, m_bLitPipeline);

    if (!renderData.DebugLines.empty()) {
        m_pImmediateContext->SetRenderTargets(1, &pActiveRTV, pActiveDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);
        m_pImmediateContext->SetViewports(1, &Viewport, pTarget->GetWidth(), pTarget->GetHeight());

        m_DebugLinePipeline.StartFrameRender(m_pImmediateContext, renderData);
        m_DebugLinePipeline.RenderLines(m_pImmediateContext, renderData.DebugLines);
    }

    // 3. Resolve the MSAA buffer into the target's actual 1-sample texture for ImGui
    if (isMSAAEnabled)
    {
        Diligent::ResolveTextureSubresourceAttribs ResolveAttribs;
        ResolveAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        ResolveAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

        m_pImmediateContext->ResolveTextureSubresource(
            m_pMSAATarget,
            pSceneRTV->GetTexture(),
            ResolveAttribs
        );
    }

    if (pShaftsRTV)
    {
        m_GodRaysPipeline.Apply(m_pImmediateContext, pPostRTV, godRaysConstants);
    }

    if (pGradeRTV)
    {
        m_ColorCorrectionPipeline.Apply(m_pImmediateContext, pTarget->GetRTV(), BuildColorCorrectionConstants(colorCorrection));
    }
}
