#include "AnimePipeline.hpp"
#include "../RenderData.hpp"
#include "../SceneConstants.hpp"

namespace Diligent {

    void AnimePipeline::CreatePasses()
    {
        if (!m_pStyleCB) m_pStyleCB = PipelineUtils::CreateDynamicCB<AnimeConstants>(m_pDevice, "Anime Constant Buffer");

        BuildMainPass(CreateModelPSOBuilder("Anime PSO", "Anime/anime_ps.hlsl", "Anime/anime_vs.hlsl")
            .InputLayout(VertexLayouts::GetAnimeLayout())
            .SampleCount(PipelineUtils::GetMSAASampleCount()));

        BuildOutlinePass(CreateModelPSOBuilder("Anime Outline PSO", "Anime/outline_ps.hlsl", "Anime/outline_vs.hlsl")
            .InputLayout(VertexLayouts::GetAnimeLayout()));
    }

    void AnimePipeline::BindModelVertexBuffers(IDeviceContext* pContext, const Model& model)
    {
        // The standard vertices in slot 0, each one's smooth normal and curvature in slot 1
        IBuffer* pBuffs[] = { model.pVertexBuffer, model.pAnimeVertexBuffer };
        pContext->SetVertexBuffers(0, 2, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    }

    void AnimePipeline::UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings)
    {
        const ProjectAnimeSettings& style = settings.anime;

        AnimeConstants constants = BuildAnimeConstants(style);
        constants.CameraPos = m_CameraPos;
        PipelineUtils::WriteCB(pContext, m_pStyleCB, constants);

        m_OutlineEnabled = style.outline_enabled && style.outline_thickness > 0.0f;
    }

}
