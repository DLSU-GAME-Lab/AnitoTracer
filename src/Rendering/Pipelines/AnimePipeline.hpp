#pragma once
#include "ScenePipeline.hpp"

namespace Diligent {
    // Anime (cel) shading, ported from Weeb_Big2 (Shaders/Anime). Forward, two passes:
    //   1. Toon pass    - two-tone lighting switched by a step on N.L, optional specular, ink along inner edges and
    //                     creases, in the style set by ProjectAnimeSettings (anime_vs/ps.hlsl)
    //   2. Outline pass - inverted hull: the mesh pushed outward and drawn in ink with front faces culled (outline_vs/ps.hlsl)
    // Both passes read a second vertex stream, Model::pAnimeVertexBuffer, which holds each vertex's smooth normal and
    // curvature (built at load by BuildAnimeVertexData).
    class AnimePipeline : public ScenePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;
        void StartFrameRender(IDeviceContext* pContext, const RenderData& renderData) override;
        void RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque = true) override;
        void UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings) override;

        // The tones are the final image, not light to be tone mapped
        bool OutputsDisplayColor() const override { return true; }

    protected:
        void BindCommonBuffers(IShaderResourceBinding* pSRB) override;
        void BindModelVertexBuffers(IDeviceContext* pContext, const Model& model) override;

    private:
        void InitializeOutlinePSO();

        // AnimeConstants, for both passes
        static std::vector<ShaderResourceVariableDesc> GetAnimeVariables();

        // From UpdateProjectSettings, for the passes in RenderModels
        bool m_OutlineEnabled = true;
        glm::vec4 m_CameraPos{ 0.0f };

        RefCntAutoPtr<IBuffer> m_pAnimeCB;

        RefCntAutoPtr<IPipelineState>         m_pOutlinePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pOutlineSRB;
    };
}
