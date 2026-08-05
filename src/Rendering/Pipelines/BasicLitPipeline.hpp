#pragma once
#include "BasePipeline.hpp"

namespace Diligent {
    class BasicLitPipeline : public BasePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, RenderData renderData) override;
    };
}