#pragma once

#include "BasePipeline.hpp"
#include "Utils/DynamicVertexBuffer.hpp"
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

        static constexpr Uint32 kMaxInstances = 1u << 20;

        // The pass that draws batches with this blend mode
        PipelinePass CreateBlendPass(ParticleBlendMode mode);

        PipelinePass m_Alpha;
        PipelinePass m_Additive;

        RefCntAutoPtr<IBuffer> m_pParticleCB;
        DynamicVertexBuffer m_Instances{ "Particle Instance Buffer", sizeof(ParticleInstance), 1024, kMaxInstances };
        RefCntAutoPtr<ITextureView> m_pWhiteSRV;

        std::vector<ParticleInstance> m_SortScratch;
    };

}
