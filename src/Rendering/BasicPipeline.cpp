#include "BasicPipeline.hpp"

void Diligent::BasicPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Basic Rendering PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    GraphicsPipeline.NumRenderTargets = 1;
    GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

    std::vector< LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("basic.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("basic.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    ShaderResourceVariableDesc Variables[] =
    {
        // DYNAMIC type allows us to update the buffer every frame or per-object
        {SHADER_TYPE_VERTEX, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    PSODesc.ResourceLayout.Variables = Variables;
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);

    // Create the Pipeline State
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    BufferDesc CBDesc;
    CBDesc.Name = "Camera Constant Buffer";
    CBDesc.Size = sizeof(float4x4);
    CBDesc.Usage = USAGE_DYNAMIC;            // Fast CPU writes
    CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    pDevice->CreateBuffer(CBDesc, nullptr, &m_pCameraCB);

    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

    if (auto* pCameraConstantsVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants"))
    {
        pCameraConstantsVar->Set(m_pCameraCB);
    }
    else
    {
        std::cout << "Variable not found" << std::endl;
    }
}

void Diligent::BasicPipeline::StartFrameRender(IDeviceContext* pContext, CameraObj camera)
{
    glm::mat4 transmat = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
    float sc = 4.f;
    transmat = glm::scale(transmat, glm::vec3(sc, sc, sc));

    camera.UpdateViewMatrix();
    camera.UpdateProjectionMatrix();

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix();

    glm::mat4 mvp = proj * view * transmat;
    glm::mat4 mvp_t = glm::transpose(mvp);

    //Forces outofscope cleanup
    {
        MapHelper<glm::mat4> CBData(pContext, m_pCameraCB, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBData = mvp_t;
    }

    pContext->SetPipelineState(m_pPSO);
    pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void Diligent::BasicPipeline::RenderModel(IDeviceContext* pContext, Model* model)
{
    // Bind vertex buffer
    IBuffer* pBuffs[] = { model->pVertexBuffer };
    pContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(model->pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    for (const auto& submesh : model->SubMeshes) {
        // --- Draw Call ---
        DrawIndexedAttribs DrawAttrs;

        // We used std::vector<Uint32> for indices during Assimp parsing
        DrawAttrs.IndexType = VT_UINT32;

        // Map the offsets from our SubMesh struct to the Draw Call attributes
        DrawAttrs.NumIndices = submesh.IndexCount;
        DrawAttrs.FirstIndexLocation = submesh.IndexOffset;
        DrawAttrs.BaseVertex = submesh.BaseVertex;

        // DRAW_FLAG_VERIFY_ALL enables debug validation in development builds
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;

        pContext->DrawIndexed(DrawAttrs);
    }
}
