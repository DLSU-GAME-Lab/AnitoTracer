#include "StylizedPipeline.hpp"
#include "../RenderData.hpp"

namespace Diligent {

    void StylizedPipeline::BindCommonBuffers(IShaderResourceBinding* pSRB)
    {
        ScenePipeline::BindCommonBuffers(pSRB);
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_VERTEX, m_StyleConstantsName, m_pStyleCB);
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, m_StyleConstantsName, m_pStyleCB);
    }

    std::vector<ShaderResourceVariableDesc> StylizedPipeline::GetStandardShaderVariables()
    {
        std::vector<ShaderResourceVariableDesc> variables = ScenePipeline::GetStandardShaderVariables();
        variables.push_back({ SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, m_StyleConstantsName, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC });
        return variables;
    }

    void StylizedPipeline::BuildOutlinePass(PSOBuilder& builder)
    {
        m_Outline = BuildPass(builder.SampleCount(PipelineUtils::GetMSAASampleCount()).Cull(CULL_MODE_FRONT));
    }

    void StylizedPipeline::StartFrameRender(IDeviceContext* pContext, const RenderData& renderData)
    {
        BasePipeline::StartFrameRender(pContext, renderData);
        m_CameraPos = renderData.Lights.CameraPos;
    }

    void StylizedPipeline::RenderModels(IDeviceContext* pContext, const RenderData& renderData, bool renderOpaque)
    {
        // StartFrameRender already set the main pass's PSO
        ScenePipeline::RenderModels(pContext, renderData, renderOpaque);

        if (!renderOpaque || !m_OutlineEnabled || !m_Outline.pPSO) return;

        pContext->SetPipelineState(m_Outline.pPSO);
        for (const ModelRenderInstance& model : renderData.Models) {
            RenderModelWithSRB(pContext, model, renderOpaque, m_Outline.pSRB);
        }
    }

}
