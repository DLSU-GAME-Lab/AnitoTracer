#include "BasicPipeline.hpp"
#include "../RenderData.hpp"

void Diligent::BasicPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Basic Rendering PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);

    std::vector<LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("basic.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("basic.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    ShaderResourceVariableDesc Variables[] =
    {
        {SHADER_TYPE_VERTEX, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VERTEX, "ModelConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}, 
        {SHADER_TYPE_PIXEL, "MaterialConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    PSODesc.ResourceLayout.Variables = Variables;
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);

    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    CreateCameraConstantBuffer(pDevice);
    CreateModelConstantBuffer(pDevice); 

    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

    if (auto* pCameraConstantsVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants"))
    {
        pCameraConstantsVar->Set(m_pCameraCB);
    }

    if (auto* pModelConstantsVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ModelConstants"))
    {
        pModelConstantsVar->Set(m_pModelCB);
    }
}

void Diligent::BasicPipeline::StartFrameRender(IDeviceContext* pContext, RenderData renderData)
{
    BasePipeline::StartFrameRender(pContext, renderData);

    pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void Diligent::BasicPipeline::RenderModel(IDeviceContext* pContext, const ModelRenderInstance model, bool renderOpaque)
{
    // Update the Model Constant Buffer with the current model's transform
    {
        MapHelper<glm::mat4> CBData(pContext, m_pModelCB, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBData = glm::transpose(model.WorldTransform);
    }

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