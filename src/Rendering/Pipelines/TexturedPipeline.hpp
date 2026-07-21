#pragma once
#include "BasePipeline.hpp"

namespace Diligent {

    class TexturedPipeline : public BasePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        // StartFrameRender is entirely handled by BasePipeline, so no override is necessary
        void RenderModel(IDeviceContext* pContext, Model* model) override;

    private:
        RefCntAutoPtr<IBuffer> m_pMaterialCB;
    };

}