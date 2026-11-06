#include "RendererManager.hpp"
#include "Pipelines/Utils/ScenePipelineRegistry.hpp"
#include "SceneConstants.hpp"
#include "../Asset/ProjectLoader.hpp"
#include "../AppState.hpp"
#include <iostream>

void RendererManager::Initialize(Diligent::IRenderDevice* pDevice, Diligent::IDeviceContext* pContext, Diligent::ISwapChain* pSwapChain, bool supportsRayTracing)
{
    m_pDevice = pDevice;
    m_pImmediateContext = pContext;
    m_pSwapChain = pSwapChain;
    m_SupportsRayTracing = supportsRayTracing;
    m_MSAAEnabled = Diligent::UserSettings::GetInstance().GetEnableMSAA();

    // Initialize pipeline based on the saved user setting
    SelectScenePipeline(Diligent::UserSettings::GetInstance().GetRendererType());

    if (!AppState::isReleaseBuild) {
        m_pEditorPipeline = Diligent::ScenePipelineRegistry::Find(Diligent::PipelineType::DEFERRED)->Create();
    }

    m_OnRendererChangeSub = gbe::ScopedSubscription::Create<RendererChangeArgs>(
        EVENT_RENDER_CHANGE,
        &RendererManager::HandleRendererChangeEvent,
        this
    );
}

void RendererManager::HandleRendererChangeEvent(const RendererChangeArgs* args)
{
    SelectScenePipeline(args->targetPipeline);
    InitializePipelines();
}

void RendererManager::SelectScenePipeline(Diligent::PipelineType requested)
{
    const Diligent::ScenePipelineInfo& selected = Diligent::ScenePipelineRegistry::Resolve(requested, m_SupportsRayTracing, m_MSAAEnabled);

    if (selected.Type != requested) {
        const Diligent::ScenePipelineInfo* pRequested = Diligent::ScenePipelineRegistry::Find(requested);
        const char* reason = (pRequested && pRequested->RequiresRayTracing && !m_SupportsRayTracing)
            ? "hardware ray tracing is not available"
            : "it does not support MSAA";
        std::cout << "[Warn] " << (pRequested ? pRequested->DisplayName : "Unknown pipeline") << " can't be used because "
            << reason << ". Falling back to " << selected.DisplayName << "." << std::endl;
    }

    Diligent::UserSettings::GetInstance().GetRendererType() = selected.Type;
    if (m_pScenePipeline && m_ScenePipelineType == selected.Type) return;

    m_pScenePipeline = selected.Create();
    m_ScenePipelineType = selected.Type;
    std::cout << "[RendererManager] Using " << selected.DisplayName << "." << std::endl;
}

void RendererManager::InitializePipelines()
{
    const auto& SCDesc = m_pSwapChain->GetDesc();

    m_pScenePipeline->InitializePipeline(m_pDevice, m_pSwapChain);
    if (m_pEditorPipeline) m_pEditorPipeline->InitializePipeline(m_pDevice, m_pSwapChain);
    m_DebugLinePipeline.InitializePipeline(m_pDevice, m_pSwapChain);
    m_ParticlePipeline.InitializePipeline(m_pDevice, m_pSwapChain);
    m_SkyboxPipeline.Initialize(m_pDevice, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat);
    m_PostProcess.Initialize(m_pDevice, SCDesc.ColorBufferFormat);
    m_GameTAA.Initialize(m_pDevice, SCDesc.ColorBufferFormat);
    m_EditorTAA.Initialize(m_pDevice, SCDesc.ColorBufferFormat);

    LoadDefaultSkybox();
}

void RendererManager::LoadDefaultSkybox()
{
    // Load your default skybox texture (Make sure it is a compiled .dds cubemap!)
    // FOR TESTING
    auto* pDefaultSkybox = ModelManager::GetInstance().LoadCubemap("Assets/Skyboxes/skybox_raw.dds", true);
    if (pDefaultSkybox) {
        m_SkyboxPipeline.SetCubemap(pDefaultSkybox);
        m_pScenePipeline->SetEnvironmentMap(pDefaultSkybox);
        if (m_pEditorPipeline) m_pEditorPipeline->SetEnvironmentMap(pDefaultSkybox);
        std::cout << "[Info] Default Skybox loaded and assigned successfully." << std::endl;
    }
    else {
        std::cout << "[Warn] Could not load Default Skybox." << std::endl;
    }
}

void RendererManager::SyncMSAAState()
{
    const bool msaaEnabled = Diligent::UserSettings::GetInstance().GetEnableMSAA();
    if (msaaEnabled == m_MSAAEnabled) return;

    m_MSAAEnabled = msaaEnabled;
    if (!msaaEnabled) m_MSAATarget.Release();

    SelectScenePipeline(Diligent::UserSettings::GetInstance().GetRendererType());
    InitializePipelines();
}

void RendererManager::OnResize(Diligent::Uint32 width, Diligent::Uint32 height)
{
    if (!m_pSwapChain) return;

    // Let the swap chain handle the raw dimensions, then use the actual resolved ones
    m_pSwapChain->Resize(width, height);
    const auto& SCDesc = m_pSwapChain->GetDesc();

    // Prevent creating 0-sized G-Buffers when minimized or transitioning
    if (SCDesc.Width == 0 || SCDesc.Height == 0) return;

    if (m_pScenePipeline) m_pScenePipeline->OnResize(SCDesc.Width, SCDesc.Height);
    if (m_pEditorPipeline) m_pEditorPipeline->OnResize(SCDesc.Width, SCDesc.Height);
}

void RendererManager::RenderFrame(const Diligent::RenderData& renderData)
{
    const auto& SCDesc = m_pSwapChain->GetDesc();
    if (SCDesc.Width == 0 || SCDesc.Height == 0) return;

    SyncMSAAState();

    // The release build is the game itself, so post processing always applies here
    const Diligent::FrameTarget backBuffer{ m_pSwapChain->GetCurrentBackBufferRTV(), m_pSwapChain->GetDepthBufferDSV(), SCDesc.Width, SCDesc.Height };
    RenderView(backBuffer, *m_pScenePipeline, renderData, true, SelectTAA(m_GameTAA, Diligent::UserSettings::GetInstance().GetTAASettings().Enabled));

    // Leave the back buffer bound for the UI
    backBuffer.Bind(m_pImmediateContext);
}

void RendererManager::RenderGameView(Diligent::RenderTarget* pTarget, const Diligent::RenderData& renderData)
{
    const Diligent::TAASettings& taa = Diligent::UserSettings::GetInstance().GetTAASettings();
    RenderToTarget(pTarget, *m_pScenePipeline, renderData, true, SelectTAA(m_GameTAA, taa.Enabled));
}

void RendererManager::RenderEditorView(Diligent::RenderTarget* pTarget, const Diligent::RenderData& renderData)
{
    if (!m_pEditorPipeline) return;
    const Diligent::TAASettings& taa = Diligent::UserSettings::GetInstance().GetTAASettings();
    RenderToTarget(pTarget, *m_pEditorPipeline, renderData, false, SelectTAA(m_EditorTAA, taa.Enabled && taa.ApplyToEditorView));
}

void RendererManager::RenderToTarget(Diligent::RenderTarget* pTarget, Diligent::ScenePipeline& scenePipeline, const Diligent::RenderData& renderData, bool applyPostProcessing, Diligent::TemporalAAPass* pTAA)
{
    if (!pTarget || !pTarget->IsValid()) return;

    SyncMSAAState();
    RenderView(pTarget->AsFrameTarget(), scenePipeline, renderData, applyPostProcessing, pTAA);
}

Diligent::TemporalAAPass* RendererManager::SelectTAA(Diligent::TemporalAAPass& pass, bool enabled)
{
    if (enabled) return &pass;
    pass.ResetHistory();
    return nullptr;
}

Diligent::ITextureView* RendererManager::GetSceneDepthSRV(const Diligent::ScenePipeline& scenePipeline, const Diligent::FrameTarget& scene)
{
    if (Diligent::ITextureView* pSRV = scenePipeline.GetDepthSRV()) return pSRV;
    if (!scene.pDSV) return nullptr;

    // The swap chain's and multisampled depth buffers can't be sampled
    Diligent::ITexture* pDepth = scene.pDSV->GetTexture();
    const Diligent::TextureDesc& desc = pDepth->GetDesc();
    if (desc.SampleCount > 1 || (desc.BindFlags & Diligent::BIND_SHADER_RESOURCE) == 0) return nullptr;
    return pDepth->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
}

void RendererManager::RenderView(const Diligent::FrameTarget& output, Diligent::ScenePipeline& scenePipeline, const Diligent::RenderData& renderData, bool applyPostProcessing, Diligent::TemporalAAPass* pTAA)
{
    const ProjectSettings& settings = ProjectLoader::GetProjectSettings();

    // Each active post effect renders into its own intermediate texture: scene -> god rays -> color correction -> output
    Diligent::ITextureView* pSceneRTV = m_PostProcess.Begin(settings, renderData, output.pRTV, output.Width, output.Height, applyPostProcessing);

    // TAA resolves into where the scene would have gone, so the scene passes draw into its input with a jittered camera instead.
    // Post effects still get the unjittered renderData, since they run on the resolved image.
    Diligent::ITextureView* pTAAOutputRTV = pSceneRTV;
    Diligent::RenderData jitteredData;
    const Diligent::RenderData* pSceneData = &renderData;
    if (pTAA) {
        const Diligent::TAASettings& taa = Diligent::UserSettings::GetInstance().GetTAASettings();
        if (Diligent::ITextureView* pTAAInputRTV = pTAA->Begin(taa, renderData.ViewMatrix, renderData.ProjectionMatrix, output.Width, output.Height)) {
            pSceneRTV = pTAAInputRTV;
            jitteredData = renderData;
            jitteredData.ProjectionMatrix = pTAA->GetJitteredProjection();
            pSceneData = &jitteredData;
        }
        else {
            pTAA = nullptr;
        }
    }

    // With MSAA the scene passes draw into the multisampled target (matching the PSOs' sample count), resolved into pSceneRTV after
    Diligent::FrameTarget scene{ pSceneRTV, output.pDSV, output.Width, output.Height };
    if (m_MSAAEnabled) {
        const auto& SCDesc = m_pSwapChain->GetDesc();
        m_MSAATarget.Create(m_pDevice, output.Width, output.Height, SCDesc.ColorBufferFormat, SCDesc.DepthBufferFormat, Diligent::PipelineUtils::GetMSAASampleCount());
        scene = m_MSAATarget.AsFrameTarget();
    }

    const float clearColor[] = { 0.1f, 0.15f, 0.25f, 1.0f };
    scene.Bind(m_pImmediateContext);
    scene.Clear(m_pImmediateContext, clearColor);

    const Diligent::FogConstants fogConstants = Diligent::BuildFogConstants(settings.fog);
    m_SkyboxPipeline.UpdateFogConstants(m_pImmediateContext, fogConstants);
    m_SkyboxPipeline.Render(m_pImmediateContext, *pSceneData);

    scenePipeline.Render(m_pImmediateContext, renderData, scene, settings, fogConstants);
    //scenePipeline.Render(m_pImmediateContext, *pSceneData, scene, settings.lighting, fogConstants);

    RenderOverlays(scene, *pSceneData);

    if (m_MSAAEnabled) ResolveMSAA(pSceneRTV->GetTexture());

    if (pTAA) pTAA->Resolve(m_pImmediateContext, GetSceneDepthSRV(scenePipeline, scene), pTAAOutputRTV);

    m_PostProcess.Apply(m_pImmediateContext);
}

void RendererManager::RenderOverlays(const Diligent::FrameTarget& target, const Diligent::RenderData& renderData)
{
    if (!renderData.Particles.empty()) {
        target.Bind(m_pImmediateContext);
        m_ParticlePipeline.RenderParticles(m_pImmediateContext, renderData);
    }

    if (!renderData.DebugLines.empty()) {
        target.Bind(m_pImmediateContext);
        m_DebugLinePipeline.StartFrameRender(m_pImmediateContext, renderData);
        m_DebugLinePipeline.RenderLines(m_pImmediateContext, renderData.DebugLines);
    }
}

void RendererManager::ResolveMSAA(Diligent::ITexture* pDestination)
{
    Diligent::ResolveTextureSubresourceAttribs ResolveAttribs;
    ResolveAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    ResolveAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    m_pImmediateContext->ResolveTextureSubresource(m_MSAATarget.GetColorTexture(), pDestination, ResolveAttribs);
}

void RendererManager::Shutdown()
{
    m_MSAATarget.Release();
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
