#include "RendererManager.hpp"
#include <iostream>
#include <type_traits>

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
    std::visit([&](auto& pipeline) {
        pipeline.InitializePipeline(m_pDevice, m_pSwapChain);
        }, m_bLitPipeline);
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
        m_pSwapChain->Resize(width, height);

        if (auto* pDeferred = std::get_if<Diligent::DeferredPipeline>(&m_bLitPipeline))
        {
            pDeferred->OnWindowResize(m_pDevice, width, height);
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

    const float clearColor[] = { 0.1f, 0.15f, 0.25f, 1.0f };
    Diligent::ITextureView* pActiveRTV = isMSAAEnabled ? m_pMSAARTV : m_pSwapChain->GetCurrentBackBufferRTV();
    Diligent::ITextureView* pActiveDSV = isMSAAEnabled ? m_pMSAADSV : m_pSwapChain->GetDepthBufferDSV();

    m_pImmediateContext->SetRenderTargets(1, &pActiveRTV, pActiveDSV, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearRenderTarget(pActiveRTV, clearColor, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_pImmediateContext->ClearDepthStencil(pActiveDSV, Diligent::CLEAR_DEPTH_FLAG, 1.0f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    std::visit([&](auto& pipeline) {
        pipeline.StartFrameRender(m_pImmediateContext, renderData);
        pipeline.UpdateLights(m_pImmediateContext, renderData.Lights);
        pipeline.UpdateShadowSettings(m_pImmediateContext, UserSettings::GetInstance().GetShadowSettings());
        pipeline.RenderModels(m_pImmediateContext, renderData, true);

        using T = std::decay_t<decltype(pipeline)>;
        if constexpr (std::is_same_v<T, Diligent::DeferredPipeline>)
        {
            pipeline.RenderLightingPass(m_pImmediateContext);
        }
        }, m_bLitPipeline);

    auto* pBackBufferRTV = m_pSwapChain->GetCurrentBackBufferRTV();
    auto* pDefaultDSV = m_pSwapChain->GetDepthBufferDSV();

    if (isMSAAEnabled)
    {
        Diligent::ResolveTextureSubresourceAttribs ResolveAttribs;
        ResolveAttribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
        ResolveAttribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

        m_pImmediateContext->ResolveTextureSubresource(
            m_pMSAATarget,
            pBackBufferRTV->GetTexture(),
            ResolveAttribs
        );
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