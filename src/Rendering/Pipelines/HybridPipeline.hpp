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

        // True when reflection rays can shade what they hit on this device (see SceneGeometryPool).
        // Otherwise the shaders are compiled without that code and reflections show the sky only.
        static bool SupportsSceneRayShading();

    protected:
        // TLAS and fallback environment map, created once and kept across re-initialization
        void InitializeRayTracingResources();

        // What the PSO of the pass that does the ray-traced shading needs: the TLAS, the sky lighting cubes and,
        // when hits can be shaded, the scene's geometry, materials and textures (Shaders/rt_scene.hlsli)
        static std::vector<ShaderResourceVariableDesc> GetRayTracingVariables();
        static std::vector<ImmutableSamplerDesc> GetRayTracingSamplers();
        static ShaderMacroList GetRayTracingMacros();

        // Binds the TLAS, sky lighting cubes and scene textures into the SRB of the ray-traced shading pass
        void BindRayTracingResources();

        // SRB of the pass that traces rays (the forward pass here, the lighting pass in DeferredHybridPipeline)
        virtual IShaderResourceBinding* GetRayTracingSRB() { return m_pSRB; }

        SceneTLAS m_SceneTLAS;
        RefCntAutoPtr<ITextureView> m_pEnvIrradianceSRV;
        RefCntAutoPtr<ITextureView> m_pEnvSpecularSRV;
        RefCntAutoPtr<ITexture> m_pFallbackEnvMap;

    private:
        // Per frame: uploads newly loaded geometry, and rebinds the scene buffers and (when it changed) the texture array
        void SyncSceneRayData(IDeviceContext* pContext);
        void BindSceneTextures();

        // SceneGeometryPool::GetTextureVersion of the array bound into GetRayTracingSRB()
        Uint64 m_BoundTextureVersion = 0;
    };
}
