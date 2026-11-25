#pragma once
#include "HybridPipeline.hpp"
#include "Utils/GBuffer.hpp"
#include "Utils/DiffuseGIPass.hpp"
#include "Utils/SignalDenoiser.hpp"

namespace Diligent {
    // Deferred version of the Hybrid pipeline:
    //   1. Depth prepass       - resolves visibility so the G-buffer is written once per pixel
    //   2. G-buffer pass       - material sampling and motion vectors (Shaders/DeferredHybrid/gbuffer_ps.hlsl)
    //   3. Guide pass          - each pixel's normal, distance and roughness, kept for a frame (guide_ps.hlsl)
    //   4. Trace passes        - one ray per pixel each, writing what the rays found as "signals":
    //                            shadows (one ray per light), indirect light (per-pixel GI through DiffuseGIPass, or
    //                            ambient occlusion for Sky + AO) and reflections
    //   5. Denoising           - each signal is averaged over frames and blurred (SignalDenoiser), unless the
    //                            project's Denoising settings switch it off
    //   6. Lighting pass       - fullscreen, composes the image from the G-buffer and the signals (lighting_ps.hlsl)
    // Rays are traced once per visible pixel, independent of overdraw. Shares its TLAS and sky lighting with HybridPipeline.
    // The signals keep one view's history: this pipeline draws one view per frame.
    class DeferredHybridPipeline : public HybridPipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData) override;
        void RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque = true) override;
        void UpdateLights(IDeviceContext* pContext, const LightConstants& lights) override;
        void UpdateShadowSettings(IDeviceContext* pContext, const ProjectSettings& settings, uint32_t noiseFrame = 0) override;
        void UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings) override;
        ITextureView* GetDepthSRV() const override { return m_GBuffer.GetDepthSRV(); }
        ITextureView* GetVelocitySRV() const override { return m_GBuffer.GetColorSRV(kGBufferVelocity); }

        // True when this device can run per-pixel GI. It shades what its rays hit, so it needs what reflections
        // of objects need. Without it the pipeline lights with Sky + AO whatever the project asks for.
        static bool SupportsPerPixelGI() { return SupportsSceneRayShading(); }

    protected:
        // Sizes the G-buffer to fit the target
        void PrepareFrame(const FrameTarget& target) override;
        // Guide, trace and denoise passes, then the lighting pass: writes color and depth into the target
        void FinishFrame(IDeviceContext* pContext, const FrameTarget& target) override;

        std::vector<IShaderResourceBinding*> GetRayTracingSRBs() override;
        void RebuildRayPipelines() override;

    private:
        // Position of the motion vector target among the G-buffer's color attachments (see InitializePipeline)
        static constexpr size_t kGBufferVelocity = 4;
        // A shadow texture holds four lights, one per channel: MAX_DIR_LIGHTS + MAX_POINT_LIGHTS fit in three
        static constexpr Uint32 kMaxShadowLayers = 3;

        void InitializeDepthPrepassPSO();
        void InitializeGBufferPSO();
        void InitializeDiffuseGI();
        void InitializeSignalPasses();
        void InitializeLightingPSO();

        // A fullscreen pass that reads the G-buffer and may trace rays: everything such a pass can use is declared,
        // and the shader keeps what it needs. Add the render targets and build.
        PSOBuilder CreateSignalPassBuilder(const char* name, const char* psFile);
        // Binds what CreateSignalPassBuilder declared that doesn't change from frame to frame
        void BindSignalPassResources(IShaderResourceBinding* pSRB);
        // The SRBs of every pass that samples the G-buffer
        std::vector<IShaderResourceBinding*> GetGBufferSRBs();

        // (Re)creates the view sized textures of the guide and trace passes; drops every history when it does
        void PrepareSignalTargets(Uint32 width, Uint32 height);
        void ResetSignalHistory();
        void DrawSignalPass(IDeviceContext* pContext, IPipelineState* pPSO, IShaderResourceBinding* pSRB, ITexture* pTarget);
        void DrawSignalPass(IDeviceContext* pContext, IPipelineState* pPSO, IShaderResourceBinding* pSRB, ITexture* pTarget0, ITexture* pTarget1);

        void SetGBufferViewport(IDeviceContext* pContext);

        RefCntAutoPtr<IPipelineState> m_pDepthPrepassPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pDepthPrepassSRB;

        RefCntAutoPtr<IPipelineState> m_pGuidePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pGuideSRB;
        RefCntAutoPtr<IPipelineState> m_pShadowTracePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pShadowTraceSRB;
        // The first shadow layer's pass when glass is in the scene: also writes the color glass tints the light with
        RefCntAutoPtr<IPipelineState> m_pShadowTraceTintPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pShadowTraceTintSRB;
        RefCntAutoPtr<IPipelineState> m_pAOTracePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pAOTraceSRB;
        RefCntAutoPtr<IPipelineState> m_pReflectionTracePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pReflectionTraceSRB;

        RefCntAutoPtr<IPipelineState> m_pLightingPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pLightingSRB;
        RefCntAutoPtr<IBuffer> m_pDeferredCB;

        // Layout documented in Shaders/DeferredHybrid/gbuffer_common.hlsli. Only grows, so one set serves every
        // view (editor, game, swap chain); passes render into its top-left viewport-sized corner.
        GBuffer m_GBuffer;

        // View sized, see Shaders/DeferredHybrid/deferred_common.hlsli for what each holds.
        // The guide is ping-ponged: one entry holds last frame's while the other is written.
        RefCntAutoPtr<ITexture> m_pGuide[2];
        Uint32 m_GuideCurrent = 0;              // Entry written by the last FinishFrame
        bool m_GuideValid = false;              // False until a frame has been drawn at this size
        RefCntAutoPtr<ITexture> m_pShadowRays[kMaxShadowLayers]; // Created when the scene has lights for them
        RefCntAutoPtr<ITexture> m_pAORays;      // Created when Sky + AO is in use, at the view's size or half of it
        RefCntAutoPtr<ITexture> m_pReflectionRays;
        RefCntAutoPtr<ITexture> m_pGlassTintRays;
        Uint32 m_SignalWidth = 0;
        Uint32 m_SignalHeight = 0;

        // Traces the indirect signal for the GI method Per-pixel
        DiffuseGIPass m_GI;

        // One denoiser for every signal, and what each signal remembers between frames
        SignalDenoiser m_Denoiser;
        SignalDenoiser::History m_ShadowHistory[kMaxShadowLayers];
        SignalDenoiser::History m_IndirectHistory;
        SignalDenoiser::History m_ReflectionHistory;
        SignalDenoiser::History m_TintHistory;

        // Bound in place of signals that aren't traced: no shadow (white), no reflection (black)
        RefCntAutoPtr<ITexture> m_pWhiteTexture;
        RefCntAutoPtr<ITexture> m_pBlackTexture;

        // From UpdateProjectSettings / UpdateLights / UpdateShadowSettings, for the passes in FinishFrame
        ProjectGISettings m_GISettings;
        ProjectDenoiseSettings m_DenoiseSettings;
        ProjectReflectionSettings m_ReflectionSettings;
        float m_ReflectionIntensity = 1.0f;
        int m_NumDirLights = 0;
        int m_NumPointLights = 0;
        Uint32 m_NoiseFrame = 0;

        // What last frame's histories were gathered for; a change makes them meaningless
        int m_HistoryDirLights = -1;
        int m_HistoryPointLights = -1;
        int m_HistoryIndirectMethod = -1;
        Uint32 m_HistoryIndirectScale = 0;

        // Counts the frames drawn: moves the rays of the signals a denoiser averages
        Uint32 m_SignalFrame = 0;

        // Size of the current frame's target
        Uint32 m_ViewportWidth = 0;
        Uint32 m_ViewportHeight = 0;

        // This frame's camera, kept from StartFrameRender for the passes in FinishFrame, and last frame's position
        glm::mat4 m_ViewProj{ 1.0f };
        glm::vec3 m_CameraPos{ 0.0f };
        glm::vec3 m_PrevCameraPos{ 0.0f };
        float m_PixelAngle = 0.0f;
    };
}
