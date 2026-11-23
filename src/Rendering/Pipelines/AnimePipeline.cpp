#include "AnimePipeline.hpp"
#include "../RenderData.hpp"
#include "../SceneConstants.hpp"

namespace Diligent {

    void AnimePipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
    {
        SetDevice(pDevice, pSwapChain);
        CreateCommonConstantBuffers();
        if (!m_pAnimeCB) m_pAnimeCB = PipelineUtils::CreateDynamicCB<AnimeConstants>(m_pDevice, "Anime Constant Buffer");

        m_pPSO = CreateModelPSOBuilder("Anime PSO", "Anime/anime_ps.hlsl", "Anime/anime_vs.hlsl")
            .InputLayout(VertexLayouts::GetAnimeLayout())
            .SampleCount(PipelineUtils::GetMSAASampleCount())
            .Variables(GetAnimeVariables())
            .Build(pDevice);
        m_pSRB = PipelineUtils::CreateSRB(m_pPSO);
        BindCommonBuffers(m_pSRB);

        InitializeOutlinePSO();
    }

    void AnimePipeline::InitializeOutlinePSO()
    {
        m_pOutlinePSO = CreateModelPSOBuilder("Anime Outline PSO", "Anime/outline_ps.hlsl", "Anime/outline_vs.hlsl")
            .InputLayout(VertexLayouts::GetAnimeLayout())
            .SampleCount(PipelineUtils::GetMSAASampleCount())
            .Cull(CULL_MODE_FRONT)
            .Variables(GetAnimeVariables())
            .Build(m_pDevice);
        m_pOutlineSRB = PipelineUtils::CreateSRB(m_pOutlinePSO);
        BindCommonBuffers(m_pOutlineSRB);
    }

    std::vector<ShaderResourceVariableDesc> AnimePipeline::GetAnimeVariables()
    {
        return {
            { SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, "AnimeConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }
        };
    }

    void AnimePipeline::BindCommonBuffers(IShaderResourceBinding* pSRB)
    {
        ScenePipeline::BindCommonBuffers(pSRB);
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_VERTEX, "AnimeConstants", m_pAnimeCB);
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "AnimeConstants", m_pAnimeCB);
    }

    void AnimePipeline::BindModelVertexBuffers(IDeviceContext* pContext, const Model& model)
    {
        // The standard vertices in slot 0, each one's smooth normal and curvature in slot 1
        IBuffer* pBuffs[] = { model.pVertexBuffer, model.pAnimeVertexBuffer };
        pContext->SetVertexBuffers(0, 2, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    }

    void AnimePipeline::StartFrameRender(IDeviceContext* pContext, const RenderData& renderData)
    {
        BasePipeline::StartFrameRender(pContext, renderData);
        m_CameraPos = renderData.Lights.CameraPos;
    }

    void AnimePipeline::UpdateProjectSettings(IDeviceContext* pContext, const ProjectSettings& settings)
    {
        const ProjectAnimeSettings& style = settings.anime;

        AnimeConstants constants = BuildAnimeConstants(style);
        constants.CameraPos = m_CameraPos;
        PipelineUtils::WriteCB(pContext, m_pAnimeCB, constants);

        m_OutlineEnabled = style.outline_enabled && style.outline_thickness > 0.0f;
    }

    void AnimePipeline::RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque)
    {
        // StartFrameRender already set the toon PSO
        ScenePipeline::RenderModels(pContext, renderData, renderOpaque);

        if (!renderOpaque || !m_OutlineEnabled || !m_pOutlinePSO) return;

        pContext->SetPipelineState(m_pOutlinePSO);
        for (const ModelRenderInstance& model : renderData.Models) {
            RenderModelWithSRB(pContext, model, renderOpaque, m_pOutlineSRB);
        }
    }

}
