#include "TexturedPipeline.hpp"
#include "../RenderData.hpp"

void Diligent::TexturedPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Textured Rendering PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);

    std::vector< LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("texturedBasic.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("texturedBasic.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    ShaderResourceVariableDesc Variables[] =
    {
        {SHADER_TYPE_VERTEX, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_VERTEX, "ModelConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}, // Added Model buffer
        {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "MaterialConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    PSODesc.ResourceLayout.Variables = Variables;
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);

    SamplerDesc SamLinearWrapDesc = GetLinearWrapSamplerDesc();

    ImmutableSamplerDesc ImtblSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_Texture_sampler", SamLinearWrapDesc}
    };
    PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
    PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    CreateCameraConstantBuffer(pDevice);
    CreateModelConstantBuffer(pDevice); // Initialize the new Model buffer

    BufferDesc MatCBDesc;
    MatCBDesc.Name = "Material Constant Buffer";
    MatCBDesc.Size = sizeof(float4);
    MatCBDesc.Usage = USAGE_DYNAMIC;
    MatCBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    MatCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(MatCBDesc, nullptr, &m_pMaterialCB);

    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);
    if (auto* pCameraConstantsVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) {
        pCameraConstantsVar->Set(m_pCameraCB);
    }

    if (auto* pModelConstantsVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ModelConstants")) {
        pModelConstantsVar->Set(m_pModelCB);
    }

    if (auto* pMatConstantsVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "MaterialConstants")) {
        pMatConstantsVar->Set(m_pMaterialCB);
    }
}

void Diligent::TexturedPipeline::RenderModel(IDeviceContext* pContext, const ModelRenderInstance modelData, bool renderOpaque)
{
    Model* model = modelData.ModelData;

    // Update the Model Constant Buffer with the current model's transform
    {
        MapHelper<glm::mat4> CBData(pContext, m_pModelCB, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBData = glm::transpose(modelData.WorldTransform);
    }

    // Bind vertex and index buffers
    IBuffer* pBuffs[] = { model->pVertexBuffer };
    pContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(model->pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    for (const auto& submesh : model->SubMeshes) {

        // Bind the specific material texture for this submesh
        if (submesh.MaterialIndex < model->PBRMaterials.size()) {
            if (auto pTextureView = model->PBRMaterials[submesh.MaterialIndex].BaseColor) {
                m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pTextureView);
            }

            //Map and update the solid color for this specific material
            {
                MapHelper<float4> CBData(pContext, m_pMaterialCB, MAP_WRITE, MAP_FLAG_DISCARD);
                *CBData = model->MaterialColors[submesh.MaterialIndex];
            }
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