#pragma once

#include "BasePipeline.hpp"
#include "../ParticleRenderData.hpp"
#include <vector>

namespace Diligent {

    /// Standalone camera-facing billboard pass. Draws RenderData::Particles after the lit
    /// geometry, with depth test but no depth write, using alpha or additive blending.
    class ParticlePipeline : public BasePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;

        // Renders every batch. Render targets / viewport must already be bound.
        void RenderParticles(IDeviceContext* pContext, const RenderData& renderData);

    private:
        struct ParticleConstants {
            glm::vec4 CameraRight;
            glm::vec4 CameraUp;
            glm::vec4 Grid;
        };

        struct BlendState {
            RefCntAutoPtr<IPipelineState> PSO;
            RefCntAutoPtr<IShaderResourceBinding> SRB;
        };

        void CreateBlendState(IRenderDevice* pDevice, BlendState& state, ParticleBlendMode mode);
        void CreateWhiteTexture(IRenderDevice* pDevice);
        void EnsureInstanceBufferCapacity(Uint32 requiredInstances);

        BlendState m_Alpha;
        BlendState m_Additive;

        RefCntAutoPtr<IBuffer> m_pParticleCB;
        RefCntAutoPtr<IBuffer> m_pInstanceBuffer;
        Uint32 m_InstanceCapacity = 0;
        RefCntAutoPtr<ITextureView> m_pWhiteSRV;

        std::vector<ParticleInstance> m_SortScratch;
    };

}
