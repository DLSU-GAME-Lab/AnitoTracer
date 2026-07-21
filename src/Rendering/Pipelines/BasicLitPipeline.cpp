#include "BasicLitPipeline.hpp"
#include "../RenderData.hpp"

void Diligent::LitPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Lit Rendering PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);

    // Add this line to enable 4x MSAA for this pipeline
    GraphicsPipeline.SmplDesc.Count = 4;

    std::vector<LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("litTextured.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("litTextured.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    //Added or (|) since vs and ps shaders are combined in a single file
    //Also to get away with binding errors due to strictness
    ShaderResourceVariableDesc Variables[] =
    {
        {SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, "ModelConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, "LightConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, "MaterialConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    PSODesc.ResourceLayout.Variables = Variables;
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);

    SamplerDesc SamLinearWrapDesc = GetLinearWrapSamplerDesc();

    ImmutableSamplerDesc ImtblSamplers[] =
    {
        {SHADER_TYPE_PIXEL | SHADER_TYPE_VERTEX, "g_Texture_sampler", SamLinearWrapDesc}
    };
    PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
    PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    // Initialize Constant Buffers
    CreateCameraConstantBuffer(pDevice);
    CreateModelConstantBuffer(pDevice);

    BufferDesc MatCBDesc;
    MatCBDesc.Name = "Material Constant Buffer";
    MatCBDesc.Size = sizeof(float4);
    MatCBDesc.Usage = USAGE_DYNAMIC;
    MatCBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    MatCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(MatCBDesc, nullptr, &m_pMaterialCB);

    BufferDesc LightCBDesc;
    LightCBDesc.Name = "Light Constant Buffer";
    LightCBDesc.Size = sizeof(LightConstants);
    LightCBDesc.Usage = USAGE_DYNAMIC;
    LightCBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    LightCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(LightCBDesc, nullptr, &m_pLightCB);

    // Bind Buffers to SRB
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

    if (auto* pCameraVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) {
        pCameraVar->Set(m_pCameraCB);
    }
    if (auto* pModelVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ModelConstants")) {
        pModelVar->Set(m_pModelCB);
    }
    if (auto* pLightVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "LightConstants")) {
        pLightVar->Set(m_pLightCB);
    }
    if (auto* pMatVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "MaterialConstants")) {
        pMatVar->Set(m_pMaterialCB);
    }
}

void Diligent::LitPipeline::UpdateLights(IDeviceContext* pContext, const LightConstants& lights)
{
    MapHelper<LightConstants> CBData(pContext, m_pLightCB, MAP_WRITE, MAP_FLAG_DISCARD);
    *CBData = lights;
}

void Diligent::LitPipeline::RenderModel(IDeviceContext* pContext, const ModelRenderInstance modelData)
{
    Model* model = modelData.ModelData;

    {
        MapHelper<glm::mat4> CBData(pContext, m_pModelCB, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBData = glm::transpose(modelData.WorldTransform);
    }

    IBuffer* pBuffs[] = { model->pVertexBuffer };
    pContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(model->pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    for (const auto& submesh : model->SubMeshes) {
        if (submesh.MaterialIndex < model->PBRMaterials.size()) {
            if (auto pTextureView = model->PBRMaterials[submesh.MaterialIndex].BaseColor) {
                m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pTextureView);
            }

            {
                MapHelper<float4> CBData(pContext, m_pMaterialCB, MAP_WRITE, MAP_FLAG_DISCARD);
                *CBData = model->MaterialColors[submesh.MaterialIndex];
            }
        }

        pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.NumIndices = submesh.IndexCount;
        DrawAttrs.FirstIndexLocation = submesh.IndexOffset;
        DrawAttrs.BaseVertex = submesh.BaseVertex;
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;

        pContext->DrawIndexed(DrawAttrs);
    }
}