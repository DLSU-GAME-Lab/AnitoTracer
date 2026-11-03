#pragma once
#include "ScenePipeline.hpp"

namespace Diligent {
    // Forward PBR without ray tracing (Shaders/fallbackLit_ps.hlsl). Runs everywhere, so it is the final fallback.
    class BasicLitPipeline : public ScenePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
    };
}
