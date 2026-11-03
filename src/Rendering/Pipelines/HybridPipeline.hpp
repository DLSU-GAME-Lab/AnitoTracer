#pragma once
#include "ScenePipeline.hpp"
#include "Utils/SceneTLAS.hpp"

namespace Diligent {
    // Forward PBR with ray-traced shadows, sky lighting and reflections against the scene TLAS (Shaders/hybrid_ps.hlsl)
    class HybridPipeline : public ScenePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData) override;

        // Skybox cubemap used for sky lighting and reflections. nullptr falls back to black.
        void SetEnvironmentMap(ITextureView* pCubemapSRV) override;

    protected:
        // TLAS and fallback environment map, created once and kept across re-initialization
        void InitializeRayTracingResources();

        // g_TLAS and g_EnvMap variables / sampler for the PSO that does the ray-traced shading
        static std::vector<ShaderResourceVariableDesc> GetRayTracingVariables();
        static ImmutableSamplerDesc GetEnvironmentMapSampler();

        // Binds the TLAS and environment map into the SRB of the ray-traced shading pass
        void BindRayTracingResources();

        // SRB of the pass that traces rays (the forward pass here, the lighting pass in DeferredHybridPipeline)
        virtual IShaderResourceBinding* GetRayTracingSRB() { return m_pSRB; }

        ITextureView* GetEnvironmentMapView() const;

        SceneTLAS m_SceneTLAS;
        RefCntAutoPtr<ITextureView> m_pEnvMapSRV;
        RefCntAutoPtr<ITexture> m_pFallbackEnvMap;
    };
}
