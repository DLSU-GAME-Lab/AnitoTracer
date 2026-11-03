#pragma once
#include "ScenePipeline.hpp"
#include "Utils/GBuffer.hpp"

namespace Diligent {
    // Rasterized deferred shading:
    //   1. G-buffer pass   - albedo/metallic, normal/roughness and world position (Shaders/Deferred/gbuffer_ps.hlsl)
    //   2. Lighting pass   - fullscreen shading from the G-buffer into the target (Shaders/Deferred/simple_deferred_light_ps.hlsl)
    // The G-buffer is swap chain sized and single sampled, so this pipeline doesn't support MSAA for the game view.
    // The lighting pass can still write into a multisampled target, which the editor view needs since it is always deferred.
    class DeferredPipeline : public ScenePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData) override;
        void OnResize(Uint32 width, Uint32 height) override;

    protected:
        void FinishFrame(IDeviceContext* pContext, const FrameTarget& target) override;

    private:
        void InitializeLightingPSO();

        GBuffer m_GBuffer;
        RefCntAutoPtr<IPipelineState>         m_pLightingPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pLightingSRB;
    };
}
