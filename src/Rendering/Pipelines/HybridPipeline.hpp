#pragma once
#include "PipelineDefs.hpp"
#include "BasePipeline.hpp"

#include "Graphics/GraphicsEngine/interface/TopLevelAS.h"

namespace Diligent {
    class HybridPipeline : public BasePipeline {
        public:
            void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;

            void StartFrameRender(IDeviceContext* pContext, RenderData renderData) override;

            void UpdateLights(IDeviceContext* pContext, const LightConstants& lights);
            void UpdateShadowSettings(IDeviceContext* pContext, const ShadowSettings& settings);

            void RenderModel(IDeviceContext* pContext, const ModelRenderInstance model) override;

        private:
            void InitializeTLAS(IRenderDevice* pDevice, Uint32 maxInstances = 1000);
            void BuildSceneTLAS(IDeviceContext* pContext, const RenderData& renderData);

            RefCntAutoPtr<IBuffer> m_pMaterialCB;
            RefCntAutoPtr<IBuffer> m_pLightCB;
            RefCntAutoPtr<IBuffer> m_pShadowCB;

            RefCntAutoPtr<ITopLevelAS> m_pTLAS;
            RefCntAutoPtr<IBuffer> m_pTLASScratchBuffer;
            RefCntAutoPtr<IBuffer> m_pTLASInstanceBuffer;
    };
}
