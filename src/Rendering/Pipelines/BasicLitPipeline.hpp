#pragma once
#include "PipelineDefs.hpp"
#include "BasePipeline.hpp"

namespace Diligent {
    class LitPipeline : public BasePipeline {
        public:
            void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
            void UpdateLights(IDeviceContext* pContext, const LightConstants& lights);
            void RenderModel(IDeviceContext* pContext, const ModelRenderInstance model) override;

        private:
            RefCntAutoPtr<IBuffer> m_pMaterialCB;
            RefCntAutoPtr<IBuffer> m_pLightCB;
    };
}
