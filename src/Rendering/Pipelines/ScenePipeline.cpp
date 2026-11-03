#include "ScenePipeline.hpp"
#include "../SceneConstants.hpp"

namespace Diligent {

    void ScenePipeline::Render(IDeviceContext* pContext, const RenderData& renderData, const FrameTarget& target,
        const ProjectLightingSettings& lighting, const FogConstants& fog)
    {
        PrepareFrame(target);

        StartFrameRender(pContext, renderData);
        UpdateLights(pContext, renderData.Lights);
        UpdateShadowSettings(pContext, lighting);
        UpdateFogConstants(pContext, fog);
        RenderModels(pContext, renderData, true);

        FinishFrame(pContext, target);
    }

    void ScenePipeline::UpdateLights(IDeviceContext* pContext, const LightConstants& lights)
    {
        PipelineUtils::WriteCB(pContext, m_pLightCB, lights);
    }

    void ScenePipeline::UpdateShadowSettings(IDeviceContext* pContext, const ProjectLightingSettings& settings)
    {
        PipelineUtils::WriteCB(pContext, m_pShadowCB, BuildShadowSettings(settings));
    }

    void ScenePipeline::UpdateFogConstants(IDeviceContext* pContext, const FogConstants& fog)
    {
        PipelineUtils::WriteCB(pContext, m_pFogCB, fog);
    }

    void ScenePipeline::CreateCommonConstantBuffers()
    {
        CreateCameraConstantBuffer();
        CreateModelConstantBuffer();
        CreateMaterialConstantBuffer();

        if (!m_pLightCB) m_pLightCB = PipelineUtils::CreateDynamicCB<LightConstants>(m_pDevice, "Light Constant Buffer");
        if (!m_pShadowCB) m_pShadowCB = PipelineUtils::CreateDynamicCB<ShadowSettings>(m_pDevice, "Shadow Settings Constant Buffer");
        if (!m_pFogCB) m_pFogCB = PipelineUtils::CreateDynamicCB<FogConstants>(m_pDevice, "Fog Constant Buffer");
    }

    void ScenePipeline::BindCommonBuffers(IShaderResourceBinding* pSRB)
    {
        BasePipeline::BindCommonBuffers(pSRB);
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "LightConstants", m_pLightCB);
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "ShadowSettings", m_pShadowCB);
        PipelineUtils::BindVariable(pSRB, SHADER_TYPE_PIXEL, "FogConstants", m_pFogCB);
    }

}
