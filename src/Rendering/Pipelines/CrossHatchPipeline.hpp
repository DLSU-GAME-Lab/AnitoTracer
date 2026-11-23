#pragma once
#include "ScenePipeline.hpp"
#include "Utils/SceneTLAS.hpp"

namespace Diligent {
    // Non-photorealistic cross hatching, ported from AnitoCrossHatch (Shaders/CrossHatch). Forward, two passes:
    //   1. Hatch pass   - Lambert lighting turned into ink strokes, in the style set by ProjectCrossHatchSettings (crosshatch_ps.hlsl)
    //   2. Outline pass - inverted hull: the mesh pushed outward and drawn in ink with front faces culled (outline_vs/ps.hlsl)
    // With rayTracedShadows the hatch pass traces shadow rays against the scene TLAS (crosshatch_rt_ps.hlsl), so cast shadows
    // darken the lighting and get their own hatch layer, as the shadow map did in the original.
    class CrossHatchPipeline : public ScenePipeline {
    public:
        explicit CrossHatchPipeline(bool rayTracedShadows = false) : m_RayTracedShadows(rayTracedShadows) {}

        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData) override;
        void RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque = true) override;
        void UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings) override;

        // Ink and paper colors are the final image, not light to be tone mapped
        bool OutputsDisplayColor() const override { return true; }

    protected:
        void BindCommonBuffers(IShaderResourceBinding* pSRB) override;

    private:
        void InitializeOutlinePSO();

        // Stroke texture for the toon texture mode, loaded once
        void LoadHatchTexture();

        // Binds Assets/CrossHatch/Noise/noise<index>.dds if it isn't bound already
        void BindNoiseTexture(int index);

        // CrossHatchConstants plus the noise and stroke textures, for both passes
        static std::vector<ShaderResourceVariableDesc> GetCrossHatchVariables();

        const bool m_RayTracedShadows;

        // From UpdateProjectSettings, for the passes in RenderModels
        bool m_OutlineEnabled = true;
        int m_BoundNoiseTexture = 0;
        glm::vec4 m_CameraPos{ 0.0f };

        SceneTLAS m_SceneTLAS;
        RefCntAutoPtr<IBuffer> m_pCrossHatchCB;
        ITextureView* m_pHatchTexture = nullptr;        // Owned by ModelManager's cache
        RefCntAutoPtr<ITexture> m_pFallbackTexture;     // White, if a style texture is missing

        RefCntAutoPtr<IPipelineState>         m_pOutlinePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pOutlineSRB;
    };
}
