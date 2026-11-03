#include "TexturedPipeline.hpp"
#include "../RenderData.hpp"

void Diligent::TexturedPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    SetDevice(pDevice, pSwapChain);
    CreateCameraConstantBuffer();
    CreateModelConstantBuffer();
    if (!m_pColorCB) m_pColorCB = PipelineUtils::CreateDynamicCB<float4>(pDevice, "Material Constant Buffer");

    m_pPSO = PSOBuilder("Textured Rendering PSO")
        .Shaders("texturedBasic.hlsl", "texturedBasic.hlsl")
        .RenderTargets({ GetColorFormat() }, GetDepthFormat())
        .InputLayout(VertexLayouts::GetStandardLayout())
        .Variable(SHADER_TYPE_VERTEX, "CameraConstants")
        .Variable(SHADER_TYPE_VERTEX, "ModelConstants")
        .Variable(SHADER_TYPE_PIXEL, "g_Texture")
        .Variable(SHADER_TYPE_PIXEL, "MaterialConstants")
        .Sampler(SHADER_TYPE_PIXEL, "g_Texture_sampler", PipelineUtils::LinearWrapSampler())
        .Build(pDevice);
    m_pSRB = PipelineUtils::CreateSRB(m_pPSO);
    BindCommonBuffers(m_pSRB);
    PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "MaterialConstants", m_pColorCB);
}

void Diligent::TexturedPipeline::RenderModel(IDeviceContext* pContext, const ModelRenderInstance& modelData, bool renderOpaque)
{
    Model* model = modelData.ModelData;

    // Update the Model Constant Buffer with the current model's transform
    PipelineUtils::WriteCB(pContext, m_pModelCB, glm::transpose(modelData.WorldTransform));

    // Bind vertex and index buffers
    IBuffer* pBuffs[] = { model->pVertexBuffer };
    pContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(model->pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    for (const auto& submesh : model->SubMeshes) {

        // Bind the specific material texture and solid color for this submesh
        if (submesh.MaterialIndex < model->PBRMaterials.size()) {
            if (auto pTextureView = model->PBRMaterials[submesh.MaterialIndex].BaseColor) {
                PipelineUtils::BindVariable(m_pSRB, SHADER_TYPE_PIXEL, "g_Texture", pTextureView);
            }

            PipelineUtils::WriteCB(pContext, m_pColorCB, model->MaterialColors[submesh.MaterialIndex]);
        }

        // Commit resources right before the draw call so the new texture is active
        pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // --- Draw Call ---
        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.NumIndices = submesh.IndexCount;
        DrawAttrs.FirstIndexLocation = submesh.IndexOffset;
        DrawAttrs.BaseVertex = submesh.BaseVertex;
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;

        pContext->DrawIndexed(DrawAttrs);
    }
}
