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
    // other surfaces and from the sky, gathered with a few rays per pixel (Shaders/DeferredHybrid/gi_trace_ps.hlsl).
    //
    // This pass only traces: cosine-weighted rays per GI pixel from the G-buffer; a hit is shaded like a reflection
    // hit and also lit by last frame's GI there (so light bounces on, frame after frame), a miss returns the sky.
    // A few rays are far too little on their own, so the owner hands the result to a SignalDenoiser, which averages
    // it over time and blurs it, and then to its lighting pass, which scales it up when it was traced at half resolution.
    //
    // It keeps one view's history, like TemporalAAPass: a pipeline that draws several views needs one per view.
    class DiffuseGIPass {
    public:
        // The trace pass shades ray hits, so its PSO needs everything the owner's ray-traced passes have
        // (HybridPipeline::GetRayTracingVariables / Samplers / Macros)
        struct TraceSetup {
            std::vector<ShaderResourceVariableDesc> Variables;
            std::vector<ImmutableSamplerDesc> Samplers;
            ShaderMacroList Macros;
        };

        void Initialize(IRenderDevice* pDevice, const TraceSetup& rayTracing);
        bool IsInitialized() const { return m_pTracePSO != nullptr; }

        // The owner binds into this what its own ray-traced passes get: TLAS, sky cubes, scene buffers and textures,
        // light and shadow constants, and the G-buffer (g_GBufferNormals, g_GBufferDepth)
        IShaderResourceBinding* GetTraceSRB() const { return m_pTraceSRB; }

        struct View {
            glm::mat4 ViewProj{ 1.0f };     // The projection * view the G-buffer was drawn with this frame
            glm::vec3 CameraPos{ 0.0f };
            Uint32 Width = 0;               // Size of the view in pixels (the G-buffer itself may be larger)
            Uint32 Height = 0;

            // For light that bounces more than once: what a ray hits is lit by the GI gathered there last frame.
            // pPrevGuide: last frame's guide (view sized; xy octahedral geometric normal, z distance from the
            //             camera, w > 0 where there is a surface). nullptr: there is none, rays fall back to the sky.
            // pPrevResult: last frame's GI as it was shown, i.e. after denoising. nullptr: this pass's own rays of
            //             last frame are read instead (noisier; for when nothing denoises them).
            ITextureView* pPrevGuide = nullptr;
            ITextureView* pPrevResult = nullptr;

            // True when a denoiser averages the result over frames: the rays then change every frame.
            // Otherwise they only change with NoiseFrame (RenderData::NoiseFrame: moves under TAA, 0 stays still).
            bool Denoised = false;
            Uint32 NoiseFrame = 0;
        };

        // Traces this frame's rays and returns them: rgb = indirect light arriving at the surface, a = how open its
        // surroundings are (0 enclosed .. 1 open). GetScale(settings) times smaller than the view along each axis.
        // Changes the bound render targets, so bind the frame's own afterwards.
        ITextureView* Trace(IDeviceContext* pContext, const View& view, const ProjectGISettings& settings);

        // Forgets last frame, so the next Trace doesn't read it (call on frames where GI isn't traced)
        void ResetHistory() { m_HistoryValid = false; }

        // View pixels per GI pixel along each axis
        static Uint32 GetScale(const ProjectGISettings& settings) { return settings.half_resolution ? 2u : 1u; }

    private:
        void CreateTextures(Uint32 width, Uint32 height);

        RefCntAutoPtr<IRenderDevice> m_pDevice;

        RefCntAutoPtr<IPipelineState> m_pTracePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pTraceSRB;
        RefCntAutoPtr<IBuffer> m_pConstantsCB;

        // GI sized. Ping-ponged: one holds last frame's rays while the other is written.
        RefCntAutoPtr<ITexture> m_pRays[2];
        Uint32 m_Current = 0;                       // Entry written by the last Trace
        Uint32 m_Width = 0;
        Uint32 m_Height = 0;

        bool m_HistoryValid = false;
        Uint32 m_FrameIndex = 0;
        glm::mat4 m_PrevViewProj{ 1.0f };
        glm::vec3 m_PrevCameraPos{ 0.0f };
    };

}
