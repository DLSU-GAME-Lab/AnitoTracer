#pragma once
#include "StylizedPipeline.hpp"
#include "Utils/SceneTLAS.hpp"

namespace Diligent {
    // Non-photorealistic cross hatching, ported from AnitoCrossHatch (Shaders/CrossHatch). Forward, two passes:
    //   1. Hatch pass   - Lambert lighting turned into ink strokes, in the style set by ProjectCrossHatchSettings (crosshatch_ps.hlsl)
    //   2. Outline pass - inverted hull: the mesh pushed outward and drawn in ink with front faces culled (outline_vs/ps.hlsl)
    // With rayTracedShadows the hatch pass traces shadow rays against the scene TLAS (crosshatch_rt_ps.hlsl), so cast shadows
    // darken the lighting and get their own hatch layer, as the shadow map did in the original.
    class CrossHatchPipeline : public StylizedPipeline {
    public:
        explicit CrossHatchPipeline(bool rayTracedShadows = false) : StylizedPipeline("CrossHatchConstants"), m_RayTracedShadows(rayTracedShadows) {}

        void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData) override;
        void UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings) override;

    protected:
        void CreatePasses() override;

    private:
        // Stroke texture for the toon texture mode, loaded once
        void LoadHatchTexture();

        // Binds Assets/CrossHatch/Noise/noise<index>.dds if it isn't bound already
        void BindNoiseTexture(int index);

        // The noise and stroke textures, for both passes
        static std::vector<ShaderResourceVariableDesc> GetCrossHatchVariables();

        const bool m_RayTracedShadows;

        int m_BoundNoiseTexture = 0;

        SceneTLAS m_SceneTLAS;
        ITextureView* m_pHatchTexture = nullptr;        // Owned by ModelManager's cache
        RefCntAutoPtr<ITexture> m_pFallbackTexture;     // White, if a style texture is missing
    };
}
