#pragma once
#include "ScenePipeline.hpp"
#include "Utils/SceneTLAS.hpp"

namespace Diligent {
    // Forward PBR with ray-traced shadows, sky lighting and reflections against the scene TLAS (Shaders/hybrid_ps.hlsl)
    class HybridPipeline : public ScenePipeline {
    public:
        void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData) override;
        void UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings) override;

        // Glass is drawn unless the project's Transparency settings switch it off
        bool DrawsGlass() const override;

        // Sky lighting cubes: irradiance for diffuse light, prefiltered for reflections. nullptr falls back to black.
        void SetSkyLighting(ITextureView* pIrradianceSRV, ITextureView* pSpecularSRV) override;

        // True when reflection rays can shade what they hit on this device (see SceneGeometryPool).
        // Otherwise the shaders are compiled without that code and reflections show the sky only.
        static bool SupportsSceneRayShading();

    protected:
        // Creates the TLAS and fallback environment map, then the passes that trace rays (RebuildRayPipelines)
        void CreatePasses() override;

        // Draws the blended materials, and the opaque ones that let light through (ModelRenderInstance::TransmissiveSubmeshIndices),
        // over the lit scene, farthest first (Shaders/glass_ps.hlsl). Call once the scene is in the target; does nothing
        // without glass in the frame or with glass switched off in the project.
        void RenderGlass(IDeviceContext* pContext, const FrameTarget& target);

        // Builds m_Glass with the ray tracing variables, like the pass that lights the scene
        void InitializeGlassPSO();

        // Builds the PSOs of every pass that traces rays again, with the macros as they stand now, and binds
        // their resources. Done when the scene gains or loses its cutouts and glass: see UpdateAlphaRays.
        virtual void RebuildRayPipelines();

        // The forward pass draws the glass once its opaque geometry is in
        void FinishFrame(IDeviceContext* pContext, const FrameTarget& target) override;

        // TLAS and fallback environment map, created once and kept across re-initialization
        void InitializeRayTracingResources();

        // What the PSO of the pass that does the ray-traced shading needs: the TLAS, the sky lighting cubes and,
        // when hits can be shaded, the scene's geometry, materials and textures (Shaders/rt_scene.hlsli)
        static std::vector<ShaderResourceVariableDesc> GetRayTracingVariables();
        static std::vector<ImmutableSamplerDesc> GetRayTracingSamplers();
        // RT_ALPHA_TEST is among them while the scene holds cutouts or glass (see UpdateAlphaRays)
        ShaderMacroList GetRayTracingMacros() const;

        // Binds the TLAS, sky lighting cubes and scene textures into the SRBs of the passes that trace rays
        void BindRayTracingResources();

        // SRBs of the passes that trace rays, each built with GetRayTracingVariables: the forward pass here,
        // the lighting and GI passes in DeferredHybridPipeline. Null entries are skipped.
        virtual std::vector<IShaderResourceBinding*> GetRayTracingSRBs() { return { m_pSRB, m_Glass.pSRB }; }

        SceneTLAS m_SceneTLAS;
        PipelinePass m_Glass;
        ProjectTransparencySettings m_TransparencySettings;     // From UpdateProjectSettings
        const RenderData* m_pFrameData = nullptr;               // This frame's, from StartFrameRender until the frame is done
        RefCntAutoPtr<ITextureView> m_pEnvIrradianceSRV;
        RefCntAutoPtr<ITextureView> m_pEnvSpecularSRV;
        RefCntAutoPtr<ITexture> m_pFallbackEnvMap;

    private:
        // Compiles the code that lets rays judge cutouts and glass in while the scene holds any, and out again once it has
        // not for a while. Rebuilds the ray passes' PSOs when that changes, which hitches for a moment.
        void UpdateAlphaRays();
        bool m_AlphaRays = false;
        int m_FramesWithoutAlphaGeometry = 0;

        // Per frame: uploads newly loaded geometry, and rebinds the scene buffers and (when it changed) the texture array
        void SyncSceneRayData(IDeviceContext* pContext);
        void BindSceneTextures();

        // SceneGeometryPool::GetTextureVersion of the array bound into GetRayTracingSRBs()

        Uint64 m_BoundTextureVersion = 0;
    };
}
