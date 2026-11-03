#pragma once
#include "HybridPipeline.hpp"
#include "Utils/GBuffer.hpp"

namespace Diligent {
    // Deferred version of the Hybrid pipeline:
    //   1. Depth prepass       - resolves visibility so the G-buffer is written once per pixel
    //   2. G-buffer pass       - material sampling only (Shaders/DeferredHybrid/gbuffer_ps.hlsl)
    //   3. Lighting pass       - fullscreen, ray-traced shading from the G-buffer (Shaders/DeferredHybrid/lighting_ps.hlsl)
    // Rays are traced once per visible pixel, independent of overdraw. Shares its TLAS and sky lighting with HybridPipeline.
    class DeferredHybridPipeline : public HybridPipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData) override;
        void RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque = true) override;

    protected:
        // Sizes the G-buffer to fit the target
        void PrepareFrame(const FrameTarget& target) override;
        // Lighting pass: writes color and depth into the target
        void FinishFrame(IDeviceContext* pContext, const FrameTarget& target) override;

        IShaderResourceBinding* GetRayTracingSRB() override { return m_pLightingSRB; }

    private:
        void InitializeDepthPrepassPSO();
        void InitializeGBufferPSO();
        void InitializeLightingPSO();

        void SetGBufferViewport(IDeviceContext* pContext);

        RefCntAutoPtr<IPipelineState> m_pDepthPrepassPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pDepthPrepassSRB;

        RefCntAutoPtr<IPipelineState> m_pLightingPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pLightingSRB;
        RefCntAutoPtr<IBuffer> m_pDeferredCB;

        // Layout documented in Shaders/DeferredHybrid/gbuffer_common.hlsli. Only grows, so one set serves every
        // view (editor, game, swap chain); passes render into its top-left viewport-sized corner.
        GBuffer m_GBuffer;

        // Size of the current frame's target
        Uint32 m_ViewportWidth = 0;
        Uint32 m_ViewportHeight = 0;
    };
}
