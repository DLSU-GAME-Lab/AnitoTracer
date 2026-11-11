#pragma once
#include "ScenePipeline.hpp"
#include "Utils/SceneTLAS.hpp"

namespace Diligent {
    // Forward PBR with ray-traced shadows, sky lighting and reflections against the scene TLAS (Shaders/hybrid_ps.hlsl)
    class HybridPipeline : public ScenePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData) override;

        // Sky lighting cubes: irradiance for diffuse light, prefiltered for reflections. nullptr falls back to black.
        void SetSkyLighting(ITextureView* pIrradianceSRV, ITextureView* pSpecularSRV) override;

    protected:
        // TLAS and fallback environment map, created once and kept across re-initialization
        void InitializeRayTracingResources();

        // g_TLAS and the sky lighting cube variables / sampler for the PSO that does the ray-traced shading
        static std::vector<ShaderResourceVariableDesc> GetRayTracingVariables();
        static ImmutableSamplerDesc GetEnvironmentMapSampler();

        // Binds the TLAS and sky lighting cubes into the SRB of the ray-traced shading pass
        void BindRayTracingResources();

        // SRB of the pass that traces rays (the forward pass here, the lighting pass in DeferredHybridPipeline)
        virtual IShaderResourceBinding* GetRayTracingSRB() { return m_pSRB; }

        SceneTLAS m_SceneTLAS;
        RefCntAutoPtr<ITextureView> m_pEnvIrradianceSRV;
        RefCntAutoPtr<ITextureView> m_pEnvSpecularSRV;
        RefCntAutoPtr<ITexture> m_pFallbackEnvMap;
    };
}
