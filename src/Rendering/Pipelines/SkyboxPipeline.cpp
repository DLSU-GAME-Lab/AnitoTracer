#include "SkyboxPipeline.hpp"
#include "Utils/PSOBuilder.hpp"
#include "Utils/PipelineUtils.hpp"

namespace Diligent {

    void SkyboxPipeline::Initialize(IRenderDevice* pDevice, TEXTURE_FORMAT RTVFormat, TEXTURE_FORMAT DSVFormat) {
        // We render the inside of the cube, so we cull front faces.
        // Depth is read-only: the sky only draws where nothing closer is (depth <= 1.0f, the background).
        m_pPSO = PSOBuilder("Skybox PSO")
            .Shaders("Skybox/Skybox_vs.hlsl", "Skybox/Skybox_ps.hlsl")
            .RenderTargets({ RTVFormat }, DSVFormat)
            .SampleCount(PipelineUtils::GetMSAASampleCount())
            .Cull(CULL_MODE_FRONT)
            .Depth(true, false, COMPARISON_FUNC_LESS_EQUAL)
            .Variable(SHADER_TYPE_VERTEX, "CameraConstants")
            .Variable(SHADER_TYPE_PIXEL, "g_Cubemap", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE)
            .Variable(SHADER_TYPE_PIXEL, "LightConstants")
            .Variable(SHADER_TYPE_PIXEL, "FogConstants")
            .Sampler(SHADER_TYPE_PIXEL, "g_Cubemap_sampler", PipelineUtils::LinearWrapSampler())
            .Build(pDevice);
        m_pSRB = PipelineUtils::CreateSRB(m_pPSO);

        m_pTextureVar = m_pSRB ? m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Cubemap") : nullptr;

        m_pCameraCB = PipelineUtils::CreateDynamicCB<CameraConstants>(pDevice, "Skybox Camera Constant Buffer");
        m_pLightCB = PipelineUtils::CreateDynamicCB<LightConstants>(pDevice, "Skybox Light Constant Buffer");
        m_pFogCB = PipelineUtils::CreateDynamicCB<FogConstants>(pDevice, "Skybox Fog Constant Buffer");

        // Bind the buffers to the shader variables permanently
        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_VERTEX, "CameraConstants", m_pCameraCB);
        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "LightConstants", m_pLightCB);
        PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "FogConstants", m_pFogCB);
    }

    void SkyboxPipeline::UpdateFogConstants(IDeviceContext* pContext, const FogConstants& fog) {
        if (!m_pFogCB) return;

        PipelineUtils::WriteCB(pContext, m_pFogCB, fog);
    }

    void SkyboxPipeline::SetCubemap(ITextureView* pCubemapSRV) {
        if (m_pTextureVar) {
            m_pTextureVar->Set(pCubemapSRV);
        }
    }

    void SkyboxPipeline::Render(IDeviceContext* pContext, const RenderData& renderData) {
        if (!m_pPSO || !m_pSRB) return;

        // Map the view/proj matrices dynamically every frame!
        CameraConstants camera;
        camera.View = glm::transpose(renderData.ViewMatrix);
        camera.Proj = glm::transpose(renderData.ProjectionMatrix);
        PipelineUtils::WriteCB(pContext, m_pCameraCB, camera);
        PipelineUtils::WriteCB(pContext, m_pLightCB, renderData.Lights);

        pContext->SetPipelineState(m_pPSO);
        pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawAttribs DrawAttrs;
        DrawAttrs.NumVertices = 36;
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
        pContext->Draw(DrawAttrs);
    }
}
