#include "BasicPipeline.hpp"
#include "../RenderData.hpp"

void Diligent::BasicPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    SetDevice(pDevice, pSwapChain);
    CreateCameraConstantBuffer();
    CreateModelConstantBuffer();

    m_pPSO = PSOBuilder("Basic Rendering PSO")
        .Shaders("basic.hlsl", "basic.hlsl")
        .RenderTargets({ GetColorFormat() }, GetDepthFormat())
        .InputLayout(VertexLayouts::GetStandardLayout())
        .Variable(SHADER_TYPE_VERTEX, "CameraConstants")
        .Variable(SHADER_TYPE_VERTEX, "ModelConstants")
        .Variable(SHADER_TYPE_PIXEL, "MaterialConstants")
        .Build(pDevice);
    m_pSRB = PipelineUtils::CreateSRB(m_pPSO);
    BindCommonBuffers(m_pSRB);
}

void Diligent::BasicPipeline::StartFrameRender(IDeviceContext* pContext, const RenderData& renderData)
{
    BasePipeline::StartFrameRender(pContext, renderData);

    pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void Diligent::BasicPipeline::RenderModel(IDeviceContext* pContext, const ModelRenderInstance& model, bool renderOpaque)
{
    // Update the Model Constant Buffer with the current model's transform
    PipelineUtils::WriteCB(pContext, m_pModelCB, glm::transpose(model.WorldTransform));

    // Bind vertex buffer
    IBuffer* pBuffs[] = { model.ModelData->pVertexBuffer };
    pContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(model.ModelData->pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    for (const auto& submesh : model.ModelData->SubMeshes) {
        DrawIndexedAttribs DrawAttrs;

        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.NumIndices = submesh.IndexCount;
        DrawAttrs.FirstIndexLocation = submesh.IndexOffset;
        DrawAttrs.BaseVertex = submesh.BaseVertex;
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;

        pContext->DrawIndexed(DrawAttrs);
    }
}
