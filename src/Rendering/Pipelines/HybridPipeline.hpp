#pragma once
#include "BasePipeline.hpp"
#include "Graphics/GraphicsEngine/interface/TopLevelAS.h"

namespace Diligent {
    class HybridPipeline : public BasePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, RenderData renderData) override;

        // Skybox cubemap used for sky lighting and reflections. nullptr falls back to black.
        void SetEnvironmentMap(ITextureView* pCubemapSRV);

    private:
        void CreateFallbackEnvironmentMap(IRenderDevice* pDevice);

        void InitializeTLAS(IRenderDevice* pDevice, Uint32 maxInstances = 1000);
        void BuildSceneTLAS(IDeviceContext* pContext, const RenderData& renderData);

        RefCntAutoPtr<ITopLevelAS> m_pTLAS;
        RefCntAutoPtr<IBuffer> m_pTLASScratchBuffer;
        RefCntAutoPtr<IBuffer> m_pTLASInstanceBuffer;

        RefCntAutoPtr<ITextureView> m_pEnvMapSRV;
        RefCntAutoPtr<ITexture> m_pFallbackEnvMap;
    };
}