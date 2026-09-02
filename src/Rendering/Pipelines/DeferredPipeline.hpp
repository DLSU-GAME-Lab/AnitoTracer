#pragma once
#include "BasePipeline.hpp"

namespace Diligent {
    class DeferredPipeline : public BasePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, RenderData renderData) override;

        void OnWindowResize(IRenderDevice* pDevice, Uint32 Width, Uint32 Height);
        void RenderLightingPass(IDeviceContext* pContext);

    private:
        void CreateGBuffers(IRenderDevice* pDevice, Uint32 Width, Uint32 Height);
        void InitializeGBufferPSO(IRenderDevice* pDevice);
        void InitializeLightingPSO(IRenderDevice* pDevice);

        RefCntAutoPtr<IPipelineState>         m_pLightingPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pLightingSRB;

        RefCntAutoPtr<ITexture> m_pGBufferAlbedo;
        RefCntAutoPtr<ITexture> m_pGBufferNormal;
        RefCntAutoPtr<ITexture> m_pGBufferWorldPos;
        RefCntAutoPtr<ITexture> m_pDepthBuffer;
    };
}