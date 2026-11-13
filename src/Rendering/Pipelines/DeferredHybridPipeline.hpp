#pragma once
#include "HybridPipeline.hpp"
#include "Utils/GBuffer.hpp"
#include "Utils/DiffuseGIPass.hpp"

namespace Diligent {
    // Deferred version of the Hybrid pipeline:
    //   1. Depth prepass       - resolves visibility so the G-buffer is written once per pixel
    //   2. G-buffer pass       - material sampling only (Shaders/DeferredHybrid/gbuffer_ps.hlsl)
    //   3. Per-pixel GI        - indirect light gathered with one ray per pixel (DiffuseGIPass), when the project's
    //                            GI method asks for it and the device can shade ray hits
    //   4. Lighting pass       - fullscreen, ray-traced shading from the G-buffer (Shaders/DeferredHybrid/lighting_ps.hlsl)
    // Rays are traced once per visible pixel, independent of overdraw. Shares its TLAS and sky lighting with HybridPipeline.
    class DeferredHybridPipeline : public HybridPipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData) override;
        void RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque = true) override;
        void UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings) override;
        ITextureView* GetDepthSRV() const override { return m_GBuffer.GetDepthSRV(); }

        // True when this device can run per-pixel GI. It shades what its rays hit, so it needs what reflections
        // of objects need. Without it the pipeline lights with Sky + AO whatever the project asks for.
        static bool SupportsPerPixelGI() { return SupportsSceneRayShading(); }

    protected:
        // Sizes the G-buffer to fit the target
        void PrepareFrame(const FrameTarget& target) override;
        // GI passes, then the lighting pass: writes color and depth into the target
        void FinishFrame(IDeviceContext* pContext, const FrameTarget& target) override;

        std::vector<IShaderResourceBinding*> GetRayTracingSRBs() override { return { m_pLightingSRB, m_GI.GetTraceSRB() }; }

    private:
        void InitializeDepthPrepassPSO();
        void InitializeGBufferPSO();
        void InitializeDiffuseGI();
        void InitializeLightingPSO();

        void SetGBufferViewport(IDeviceContext* pContext);

        RefCntAutoPtr<IPipelineState> m_pDepthPrepassPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pDepthPrepassSRB;

        RefCntAutoPtr<IPipelineState> m_pLightingPSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pLightingSRB;
        RefCntAutoPtr<IBuffer> m_pDeferredCB;

        // Layout documented in Shaders/DeferredHybrid/gbuffer_common.hlsli. Only grows, so one set serves every
        // view (editor, game, swap chain); passes render into its top-left viewport-sized corner.
        GBuffer m_GBuffer;

        // Keeps the history of the one view this pipeline draws
        DiffuseGIPass m_GI;
        ProjectGISettings m_GISettings;
        // Bound in place of the GI textures while per-pixel GI is off; the lighting shader doesn't read it then
        RefCntAutoPtr<ITexture> m_pNoGITexture;

        // Size of the current frame's target
        Uint32 m_ViewportWidth = 0;
        Uint32 m_ViewportHeight = 0;

        // This frame's camera, kept from StartFrameRender for the passes in FinishFrame
        glm::mat4 m_ViewProj{ 1.0f };
        glm::vec3 m_CameraPos{ 0.0f };
        float m_PixelAngle = 0.0f;
    };
}
