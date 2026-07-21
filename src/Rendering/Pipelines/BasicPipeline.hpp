#pragma once

#include "BasePipeline.hpp"

namespace Diligent {

    /// <summary>
    /// Basic pipeline that only colors a solid color
    /// Gonna be my fallback pipeline asking for the bare min.
    /// </summary>
    class BasicPipeline : public BasePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, CameraObj camera) override;
        void RenderModel(IDeviceContext* pContext, Model* model) override;
    };

}