#pragma once

#include <vector>

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"
#include "Shaders/ShaderManager.hpp"
#include "../../../Asset/ProjectSettings.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>

namespace Diligent {

    // Per-pixel diffuse global illumination for a deferred pipeline: the light each visible surface receives from
    // other surfaces and from the sky, gathered with one ray per pixel (Shaders/DeferredHybrid/gi_*.hlsl).
    //
    // One ray is far too little on its own, so three passes turn it into a usable image:
    //   1. Trace       one cosine-weighted ray per GI pixel from the G-buffer; a hit is shaded like a reflection hit
    //                  and also lit by last frame's GI there (so light bounces on, frame after frame), a miss returns
    //                  the sky. Also writes the "guide": each pixel's normal and distance.
    //   2. Accumulate  blends the new rays into last frame's result, found by reprojecting each pixel. History is
    //                  dropped where the guide says last frame showed a different surface there.
    //   3. Filter      a few blur passes that stop at edges in the guide. The first one's output is what the next
    //                  frame accumulates onto.

    // The lighting pass then reads the result, scaling it up when it was traced at half resolution.
    //
    // It keeps one view's history, like TemporalAAPass: a pipeline that draws several views needs one per view.
    class DiffuseGIPass {
    public:
        // The trace pass shades ray hits, so its PSO needs everything the owner's ray-traced lighting pass has
        // (HybridPipeline::GetRayTracingVariables / Samplers / Macros)
        struct TraceSetup {
            std::vector<ShaderResourceVariableDesc> Variables;
            std::vector<ImmutableSamplerDesc> Samplers;
            ShaderMacroList Macros;
        };

        void Initialize(IRenderDevice* pDevice, const TraceSetup& rayTracing);
        bool IsInitialized() const { return m_pTracePSO && m_pAccumulatePSO && m_pFilterPSO; }

        // The owner binds into this what its own ray-traced pass gets: TLAS, sky cubes, scene buffers and textures,
        // light and shadow constants, and the G-buffer (g_GBufferNormals, g_GBufferDepth)
        IShaderResourceBinding* GetTraceSRB() const { return m_pTraceSRB; }

        struct View {
            glm::mat4 ViewProj{ 1.0f };     // The projection * view the G-buffer was drawn with this frame
            glm::vec3 CameraPos{ 0.0f };
            Uint32 Width = 0;               // Size of the view in pixels (the G-buffer itself may be larger)
            Uint32 Height = 0;
        };

        // Runs the three passes. Changes the bound render targets, so bind the frame's own afterwards.
        void Render(IDeviceContext* pContext, const View& view, const ProjectGISettings& settings);

        // Drops the accumulated light, so the next Render starts over (call on frames where GI isn't rendered)
        void ResetHistory() { m_HistoryValid = false; }

        // Results of the last Render; nullptr before the first one.
        // Result: rgb = indirect light arriving at the surface, a = how open its surroundings are (0 enclosed .. 1 open).
        // Guide:  xy = octahedral geometric normal, z = distance from the camera, w = 1 where there is a surface.
        // Both are GetScale() times smaller than the view along each axis.
        ITextureView* GetResultSRV() const { return m_pResultSRV; }
        ITextureView* GetGuideSRV() const { return m_pGuideSRV; }
        Uint32 GetScale() const { return m_Scale; }

    private:
        void CreateTextures(Uint32 width, Uint32 height);

        RefCntAutoPtr<IRenderDevice> m_pDevice;

        RefCntAutoPtr<IPipelineState> m_pTracePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pTraceSRB;
        RefCntAutoPtr<IPipelineState> m_pAccumulatePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pAccumulateSRB;
        RefCntAutoPtr<IPipelineState> m_pFilterPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pFilterSRB;
        RefCntAutoPtr<IBuffer> m_pConstantsCB;

        // All GI-sized. The pairs are ping-ponged: one holds last frame's data while the other is written.
        RefCntAutoPtr<ITexture> m_pTrace;           // This frame's rays
        RefCntAutoPtr<ITexture> m_pAccumRaw;        // Rays averaged over time, before the first blur pass
        RefCntAutoPtr<ITexture> m_pGuide[2];
        RefCntAutoPtr<ITexture> m_pAccum[2];        // The result kept for the next frame: averaged over time and blurred once
        RefCntAutoPtr<ITexture> m_pHistoryLength[2]; // How many frames each pixel has been accumulating
        RefCntAutoPtr<ITexture> m_pFiltered[2];     // Scratch for the blur passes
        Uint32 m_Current = 0;                       // Entry of the pairs written by the last Render
        Uint32 m_Width = 0;
        Uint32 m_Height = 0;
        Uint32 m_Scale = 2;

        ITexture* m_pLastResult = nullptr;          // The texture the last Render left its final result in
        ITextureView* m_pResultSRV = nullptr;

        ITextureView* m_pGuideSRV = nullptr;

        bool m_HistoryValid = false;
        Uint32 m_FrameIndex = 0;
        glm::mat4 m_PrevViewProj{ 1.0f };
        glm::vec3 m_PrevCameraPos{ 0.0f };
    };

}
