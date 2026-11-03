#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Common/interface/RefCntAutoPtr.hpp"

#include "Pipelines/ScenePipeline.hpp"
#include "Pipelines/SkyboxPipeline.hpp"
#include "Pipelines/DebugLinePipeline.hpp"
#include "Pipelines/ParticlePipeline.hpp"
#include "Pipelines/Utils/PostProcessStack.hpp"
#include "RenderData.hpp"

#include "../UserSettings.hpp"
#include "RendererEvents.hpp"
#include "RenderTarget.hpp"

// Owns the active scene pipeline and the passes around it, and draws views into the swap chain or render targets.
// A view is drawn as: skybox -> scene pipeline -> particles -> debug lines -> MSAA resolve -> post processing.
class RendererManager {
public:
    static RendererManager& GetInstance() {
        static RendererManager instance;
        return instance;
    }

    RendererManager(const RendererManager&) = delete;
    void operator=(const RendererManager&) = delete;

    void Initialize(Diligent::IRenderDevice* pDevice, Diligent::IDeviceContext* pContext, Diligent::ISwapChain* pSwapChain, bool supportsRayTracing);
    void InitializePipelines();

    void RenderFrame(const Diligent::RenderData& renderData);
    void OnResize(Diligent::Uint32 width, Diligent::Uint32 height);
    void Shutdown();

    bool IsRayTracingSupported() const { return m_SupportsRayTracing; }

    Diligent::RenderTarget* CreateRenderTarget(const std::string& name);
    //Off screen rendering to tex. Post processing (god rays, color correction) is opt-in so only the game view gets it, not the editor view.
    void RenderToTarget(Diligent::RenderTarget* pTarget, const Diligent::RenderData& renderData, bool applyPostProcessing = false);

private:
    RendererManager() = default;
    ~RendererManager() = default;

    void HandleRendererChangeEvent(const RendererChangeArgs* args);

    // Creates the closest scene pipeline to the requested one that this device and the MSAA setting support,
    // and stores the choice in UserSettings. Call InitializePipelines afterwards.
    void SelectScenePipeline(Diligent::PipelineType requested);

    // PSO sample counts are baked in, so toggling MSAA rebuilds them (switching away from pipelines that don't support it)
    void SyncMSAAState();

    void RenderView(const Diligent::FrameTarget& output, const Diligent::RenderData& renderData, bool applyPostProcessing);
    void RenderOverlays(const Diligent::FrameTarget& target, const Diligent::RenderData& renderData);
    void ResolveMSAA(Diligent::ITexture* pDestination);

    void LoadDefaultSkybox();

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_pDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pImmediateContext;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain>     m_pSwapChain;

    bool m_SupportsRayTracing = false;
    bool m_MSAAEnabled = false;

    std::unique_ptr<Diligent::ScenePipeline> m_pScenePipeline;
    Diligent::PipelineType m_ScenePipelineType = Diligent::PipelineType::BASIC_LIT;

    Diligent::SkyboxPipeline m_SkyboxPipeline;
    Diligent::ParticlePipeline m_ParticlePipeline;
    Diligent::DebugLinePipeline m_DebugLinePipeline;
    Diligent::PostProcessStack m_PostProcess; // Game output only

    // Multisampled scene color/depth, resolved into the view's target after the scene passes
    Diligent::RenderTarget m_MSAATarget{ "MSAA" };

    std::vector<std::unique_ptr<Diligent::RenderTarget>> m_RenderTargets;

    gbe::ScopedSubscription m_OnRendererChangeSub;
};
