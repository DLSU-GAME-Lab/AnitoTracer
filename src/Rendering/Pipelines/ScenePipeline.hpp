#pragma once
#include "BasePipeline.hpp"
#include "../FrameTarget.hpp"

namespace Diligent {

    // Base for the pipelines that light and draw the scene (the renderers in the Rendering menu, see ScenePipelineRegistry).
    // RendererManager draws the skybox into the target, then calls Render once per view.
    //
    // A new pipeline implements InitializePipeline (building m_pPSO / m_pSRB, usually with CreateModelPSOBuilder) and
    // overrides only the hooks it needs:
    //   PrepareFrame      - size per-view resources to the target (e.g. a G-buffer)
    //   StartFrameRender  - per-frame uploads and binding its own targets (e.g. clear and bind a G-buffer)
    //   RenderModels      - extra geometry passes (e.g. a depth prepass)
    //   FinishFrame       - passes after geometry that write into the target (e.g. deferred lighting)
    class ScenePipeline : public BasePipeline {
    public:
        // Draws the scene's opaque models into target
        void Render(IDeviceContext* pContext, const RenderData& renderData, const FrameTarget& target,
            const ProjectSettings& settings, const FogConstants& fog);

        virtual void UpdateLights(IDeviceContext* pContext, const LightConstants& lights);
        virtual void UpdateShadowSettings(IDeviceContext* pContext, const ProjectLightingSettings& settings);
        virtual void UpdateFogConstants(IDeviceContext* pContext, const FogConstants& fog);

        // Project settings specific to one pipeline (e.g. the cross hatching style)
        virtual void UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings) {}

        // Skybox cubemap, for pipelines that light with it
        virtual void SetEnvironmentMap(ITextureView* pCubemapSRV) {}

        // The swap chain was resized, for pipelines with swap chain sized resources
        virtual void OnResize(Uint32 width, Uint32 height) {}

        // Scene depth from the last Render, for pipelines that keep their own depth buffer (e.g. a G-buffer) instead of
        // drawing into the target's. Read by passes that reproject the scene, like TAA. May be larger than the view.
        virtual ITextureView* GetDepthSRV() const { return nullptr; }

    protected:
        virtual void PrepareFrame(const FrameTarget& target) {}
        virtual void FinishFrame(IDeviceContext* pContext, const FrameTarget& target) {}

        // Camera, model, material, light, shadow and fog buffers; only creates the ones that don't exist yet
        void CreateCommonConstantBuffers();
        void BindCommonBuffers(IShaderResourceBinding* pSRB) override;

        RefCntAutoPtr<IBuffer> m_pLightCB;
        RefCntAutoPtr<IBuffer> m_pShadowCB;
        RefCntAutoPtr<IBuffer> m_pFogCB;
    };

}
