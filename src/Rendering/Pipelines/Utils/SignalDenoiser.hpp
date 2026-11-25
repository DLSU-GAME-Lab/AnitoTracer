#pragma once

#include <vector>

#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/DeviceContext.h"
#include "Common/interface/RefCntAutoPtr.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>

namespace Diligent {

    // Turns a ray-traced "signal" (what one ray per pixel found: light visibility, indirect light, a reflection)
    // into a clean image. One ray per pixel is mostly noise, so two passes average many rays into each pixel:
    //   1. Temporal  blends this frame's rays into last frame's result, found through the G-buffer's motion vectors.
    //                History is dropped where the guide says last frame showed another surface there, and pulled
    //                toward this frame's rays where the surface is the same but what it shows has changed.
    //   2. Filter    a few blur passes that stop at edges in the guide and adapt to how uncertain the signal still
    //                is. The first one's output is what the next frame accumulates onto. A signal of light can ask for
    //                blur passes that keep all its light (Settings::PreserveLight).
    // These are our own filters in the style of SVGF (Shaders/DeferredHybrid/denoise_*.hlsl).
    //
    // One denoiser serves every signal of a pipeline: each signal keeps its own History, and the pipeline calls
    // Denoise once per signal and frame. Another denoiser (e.g. a library) can take this one's place per signal: the
    // pipeline only hands over the rays and gets a texture back.
    class SignalDenoiser {
    public:
        // Format of every texture a denoised signal is returned in
        static constexpr TEXTURE_FORMAT kSignalFormat = TEX_FORMAT_RGBA16_FLOAT;

        // What a signal holds, which decides how its noise is measured.
        // Mirror the SIGNAL_* constants in Shaders/DeferredHybrid/denoise_common.hlsli
        enum Kind : uint32_t {
            SIGNAL_VISIBILITY = 0,  // Four independent values in 0..1, one per channel (shadows of four lights)
            SIGNAL_LIGHT = 1,       // rgb light (or a direction), a: a fourth value blurred along with it
            SIGNAL_REFLECTION = 2,  // rgb light, a: distance to what is reflected, which sizes the blur
        };

        // What one signal remembers between frames. Keep one per signal and view.
        class History {
        public:
            // Forgets what was gathered, so the next Denoise starts over. Call on frames the signal isn't denoised.
            void Reset() { m_Valid = false; }

            // The texture the last Denoise returned; nullptr after a Reset or before the first one
            ITextureView* GetResultSRV() const;

        private:
            friend class SignalDenoiser;

            // The pairs are ping-ponged: one holds last frame's data while the other is written
            RefCntAutoPtr<ITexture> m_pSignal[2];   // The result kept for the next frame: averaged over time and blurred once
            RefCntAutoPtr<ITexture> m_pMoments[2];  // Per pixel: frames gathered, and its rays' mean and mean squared brightness
            RefCntAutoPtr<ITexture> m_pResult;      // The fully blurred result, when more than one blur pass runs
            ITexture* m_pLastResult = nullptr;
            Uint32 m_Current = 0;                   // Entry of the pairs written by the last Denoise
            Uint32 m_Width = 0;
            Uint32 m_Height = 0;
            bool m_Valid = false;
        };

        // The view being drawn, shared by every signal of the frame
        struct Frame {
            glm::mat4 ViewProj{ 1.0f };             // The projection * view the G-buffer was drawn with (jittered under TAA)
            glm::mat4 ViewProjNoJitter{ 1.0f };     // The same without TAA's jitter, as the motion vectors use
            glm::mat4 PrevViewProjNoJitter{ 1.0f }; // And last frame's
            glm::vec3 CameraPos{ 0.0f };
            glm::vec3 PrevCameraPos{ 0.0f };
            Uint32 Width = 0;                       // Size of the view in pixels
            Uint32 Height = 0;
            float PixelAngle = 0.0f;                // Angle one pixel covers, in radians

            // xy: octahedral geometric normal, z: distance from the camera, w: roughness (0: sky). View sized.
            ITextureView* pGuide = nullptr;
            ITextureView* pPrevGuide = nullptr;     // Last frame's
            // The G-buffer's motion vectors (xy, in UV units); may be larger than the view
            ITextureView* pVelocity = nullptr;
            // False when the "last frame" values above are not from the frame before (first frame, after a resize)
            bool HistoryValid = false;
        };

        struct Settings {
            Kind SignalKind = SIGNAL_LIGHT;
            float MinBlend = 0.1f;          // Smallest share a new frame gets: lower is smoother, higher reacts faster
            int FilterPasses = 3;           // Blur passes, 0 to kMaxFilterPasses. Each doubles the blur's reach.
            // Last frame's value is pulled into this many standard deviations around this frame's rays near the
            // pixel, which stops trails behind things that move. 0 trusts it as it is (for signals too noisy to tell).
            float ClampSigma = 0.0f;
            // The blur skips neighbours whose value differs by more than this many times the signal's uncertainty,
            // which keeps sharp features sharp. 0 blurs whatever lies on the same surface.
            float BrightnessSigma = 0.0f;
            float MaxRoughness = 1.0f;      // SIGNAL_REFLECTION: surfaces at least this rough got no ray
            bool VirtualMotion = false;     // SIGNAL_REFLECTION: follow the reflected image when the camera moves
            // SIGNAL_LIGHT: the blur passes exchange light between pixels instead of averaging them
            // (denoise_filter_conserve_ps.hlsl), so none of the light they blur is lost. The averaging blur loses light where a
            // few rays find a small bright area, and its first pass is fed back every frame, so the loss builds up: a small
            // glowing object lit its surroundings with about half the light it should. Costs a pass to blur the variance once,
            // and a little more in each blur pass.
            bool PreserveLight = false;
        };

        static constexpr int kMaxFilterPasses = 5;

        void Initialize(IRenderDevice* pDevice);
        bool IsInitialized() const { return m_pTemporalPSO && m_pFilterPSO; }

        // pSignal: this frame's rays, `scale` times smaller than the view along each axis (rounded up).
        // Returns the denoised signal, the same size, valid until the next Denoise with the same history.
        // Changes the bound render targets, so bind the frame's own afterwards.
        ITextureView* Denoise(IDeviceContext* pContext, const Frame& frame, History& history, ITextureView* pSignal,
            Uint32 scale, const Settings& settings);

    private:
        // Textures only used within one Denoise call, shared by every signal of the same size
        struct Scratch {
            Uint32 Width = 0;
            Uint32 Height = 0;
            RefCntAutoPtr<ITexture> pTemporal;  // The temporal pass's result, before the first blur pass
            RefCntAutoPtr<ITexture> pVariance;  // How uncertain that result is, read by every blur pass
            RefCntAutoPtr<ITexture> pVarianceBlur;  // The same blurred once, read by the blur passes that keep light
            RefCntAutoPtr<ITexture> pPing[2];   // The blur passes between the first and the last
        };

        bool PrepareHistory(History& history, Uint32 width, Uint32 height);
        Scratch* GetScratch(Uint32 width, Uint32 height);

        RefCntAutoPtr<IRenderDevice> m_pDevice;

        RefCntAutoPtr<IPipelineState> m_pTemporalPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pTemporalSRB;
        RefCntAutoPtr<IPipelineState> m_pFilterPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pFilterSRB;
        // For Settings::PreserveLight. Without them (a build that failed) such a signal gets the plain blur.
        RefCntAutoPtr<IPipelineState> m_pVariancePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pVarianceSRB;
        RefCntAutoPtr<IPipelineState> m_pFilterConservePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pFilterConserveSRB;
        RefCntAutoPtr<IBuffer> m_pConstantsCB;

        std::vector<Scratch> m_Scratch;
    };

}
