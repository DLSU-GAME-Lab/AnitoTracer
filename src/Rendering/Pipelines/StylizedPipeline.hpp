#pragma once
#include "ScenePipeline.hpp"

namespace Diligent {
    // Base for the non-photorealistic pipelines (Anime, CrossHatch). What they have in common:
    //   - a style constant buffer (the cbuffer named when constructing), read by the vertex and pixel shaders of both passes
    //   - the models drawn with the style's own shaders (the main pass), then again as an inverted-hull outline: the mesh
    //     pushed outward and drawn in ink with front faces culled
    //   - colors that are the final image, not light to be tone mapped
    //
    // A derived pipeline builds its main pass (BuildMainPass) and its outline pass (BuildOutlinePass) in CreatePasses,
    // creates m_pStyleCB, and in UpdateProjectSettings writes it and sets m_OutlineEnabled.
    class StylizedPipeline : public ScenePipeline {
    public:
        void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData) override;
        void RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque = true) override;

        // Ink and paper colors, toon tones: the final image, not light to be tone mapped
        bool OutputsDisplayColor() const override { return true; }

    protected:
        explicit StylizedPipeline(const char* styleConstantsName) : m_StyleConstantsName(styleConstantsName) {}

        // The style constants are declared and bound for the vertex and pixel stage of every pass
        void BindCommonBuffers(IShaderResourceBinding* pSRB) override;
        std::vector<ShaderResourceVariableDesc> GetStandardShaderVariables() override;

        // Builds the outline pass from the PSO builder of its shaders: culls front faces and follows the MSAA setting
        void BuildOutlinePass(PSOBuilder& builder);
        void BuildOutlinePass(PSOBuilder&& builder) { BuildOutlinePass(builder); }

        RefCntAutoPtr<IBuffer> m_pStyleCB;

        // Where the camera is this frame, for the style constants (their CameraPos)
        glm::vec4 m_CameraPos{ 0.0f };
        // Set from UpdateProjectSettings, read by RenderModels
        bool m_OutlineEnabled = true;

    private:
        const char* m_StyleConstantsName;
        PipelinePass m_Outline;
    };
}
