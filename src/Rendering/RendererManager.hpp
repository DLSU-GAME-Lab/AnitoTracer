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

// Owns the scene pipelines (the selected one for the game view, a deferred one for the editor view) and the passes around them,
// and draws views into the swap chain or render targets.
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
    // Off screen rendering to tex. The game view uses the renderer selected in the Rendering menu, with post processing (god rays, color correction).
    void RenderGameView(Diligent::RenderTarget* pTarget, const Diligent::RenderData& renderData);
    // The editor view always uses the deferred renderer, without post processing
    void RenderEditorView(Diligent::RenderTarget* pTarget, const Diligent::RenderData& renderData);

private:
    RendererManager() = default;
    ~RendererManager() = default;

    void HandleRendererChangeEvent(const RendererChangeArgs* args);

    // Creates the closest scene pipeline to the requested one that this device and the MSAA setting support,
    // and stores the choice in UserSettings. Call InitializePipelines afterwards. Only affects the game view.
    void SelectScenePipeline(Diligent::PipelineType requested);

    // PSO sample counts are baked in, so toggling MSAA rebuilds them (switching away from pipelines that don't support it)
    void SyncMSAAState();

    void RenderToTarget(Diligent::RenderTarget* pTarget, Diligent::ScenePipeline& scenePipeline, const Diligent::RenderData& renderData, bool applyPostProcessing);
    void RenderView(const Diligent::FrameTarget& output, Diligent::ScenePipeline& scenePipeline, const Diligent::RenderData& renderData, bool applyPostProcessing);
    void RenderOverlays(const Diligent::FrameTarget& target, const Diligent::RenderData& renderData);
    void ResolveMSAA(Diligent::ITexture* pDestination);

    void LoadDefaultSkybox();

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_pDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pImmediateContext;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain>     m_pSwapChain;

    bool m_SupportsRayTracing = false;
    bool m_MSAAEnabled = false;

    // Selected renderer, drawing the game view (or the back buffer in release builds)
    std::unique_ptr<Diligent::ScenePipeline> m_pScenePipeline;
    Diligent::PipelineType m_ScenePipelineType = Diligent::PipelineType::BASIC_LIT;

    // Always deferred, drawing the editor view. Not created in release builds, which have no editor view.
    std::unique_ptr<Diligent::ScenePipeline> m_pEditorPipeline;

    Diligent::SkyboxPipeline m_SkyboxPipeline;
    Diligent::ParticlePipeline m_ParticlePipeline;
    Diligent::DebugLinePipeline m_DebugLinePipeline;
    Diligent::PostProcessStack m_PostProcess; // Game output only

    // Multisampled scene color/depth, resolved into the view's target after the scene passes
    Diligent::RenderTarget m_MSAATarget{ "MSAA" };

    std::vector<std::unique_ptr<Diligent::RenderTarget>> m_RenderTargets;

    gbe::ScopedSubscription m_OnRendererChangeSub;
};
