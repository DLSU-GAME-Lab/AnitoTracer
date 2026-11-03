#pragma once
#include "HybridPipeline.hpp"

namespace Diligent {
    // Deferred version of the Hybrid pipeline:
    //   1. Depth prepass       - resolves visibility so the G-buffer is written once per pixel
    //   2. G-buffer pass       - material sampling only (Shaders/DeferredHybrid/gbuffer_ps.hlsl)
    //   3. Lighting pass       - fullscreen, ray-traced shading from the G-buffer (Shaders/DeferredHybrid/lighting_ps.hlsl)
    // Rays are traced once per visible pixel, independent of overdraw. Shares its TLAS and sky lighting with HybridPipeline.
    class DeferredHybridPipeline : public HybridPipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, RenderData renderData) override;
        void RenderModels(IDeviceContext* pContext, RenderData renderData, bool renderOpaque = true) override;
        void SetEnvironmentMap(ITextureView* pCubemapSRV) override;

        // Call before StartFrameRender. The lighting pass writes color and depth here, and the G-buffer is sized to fit it.
        void SetFrameTargets(ITextureView* pTargetRTV, ITextureView* pTargetDSV);
        void RenderLightingPass(IDeviceContext* pContext);

    private:
        void InitializeDepthPrepassPSO(IRenderDevice* pDevice);
        void InitializeGBufferPSO(IRenderDevice* pDevice);
        void InitializeLightingPSO(IRenderDevice* pDevice);
        void CreateDeferredConstantBuffer(IRenderDevice* pDevice);

        void EnsureGBufferSize(Uint32 width, Uint32 height);
        void BindGBufferToLightingSRB();
        void SetGBufferViewport(IDeviceContext* pContext);

        RefCntAutoPtr<IPipelineState> m_pDepthPrepassPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pDepthPrepassSRB;

        RefCntAutoPtr<IPipelineState> m_pLightingPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pLightingSRB;
        RefCntAutoPtr<IBuffer> m_pDeferredCB;

        // G-buffer (layout documented in Shaders/DeferredHybrid/gbuffer_common.hlsli). Only grows, so one set
        // serves every view (editor, game, swap chain); passes render into its top-left viewport-sized corner.
        RefCntAutoPtr<ITexture> m_pGBufferAlbedoMetallic;
        RefCntAutoPtr<ITexture> m_pGBufferNormals;
        RefCntAutoPtr<ITexture> m_pGBufferEmissiveAO;
        RefCntAutoPtr<ITexture> m_pGBufferRoughness;
        RefCntAutoPtr<ITexture> m_pGBufferDepth;
        Uint32 m_GBufferWidth = 0;
        Uint32 m_GBufferHeight = 0;

        // Current frame's output, set by SetFrameTargets
        ITextureView* m_pTargetRTV = nullptr;
        ITextureView* m_pTargetDSV = nullptr;
        Uint32 m_ViewportWidth = 0;
        Uint32 m_ViewportHeight = 0;
    };
}
