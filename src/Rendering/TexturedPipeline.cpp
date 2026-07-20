#include "TexturedPipeline.hpp"

void Diligent::TexturedPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    // 1. Pipeline properties[cite: 1]
    PSODesc.Name = "Textured Rendering PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    GraphicsPipeline.NumRenderTargets = 1;
    GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_LESS_EQUAL;

    std::vector< LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    // 3. Assign Shaders
    auto pVS = ShaderManager::GetInstance().GetShader("texturedBasic.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("texturedBasic.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    // 4. Define Resource Layout with Texture Support
    ShaderResourceVariableDesc Variables[] =
    {
        {SHADER_TYPE_VERTEX, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        // Add dynamic texture variable for the pixel shader
        {SHADER_TYPE_PIXEL, "g_Texture", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "MaterialConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    PSODesc.ResourceLayout.Variables = Variables;
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);

    SamplerDesc SamLinearWrapDesc;
    SamLinearWrapDesc.MinFilter = FILTER_TYPE_LINEAR;
    SamLinearWrapDesc.MagFilter = FILTER_TYPE_LINEAR;
    SamLinearWrapDesc.MipFilter = FILTER_TYPE_LINEAR;
    SamLinearWrapDesc.AddressU = TEXTURE_ADDRESS_WRAP;
    SamLinearWrapDesc.AddressV = TEXTURE_ADDRESS_WRAP;
    SamLinearWrapDesc.AddressW = TEXTURE_ADDRESS_WRAP;

    // Define a static sampler for the texture
    ImmutableSamplerDesc ImtblSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_Texture_sampler", SamLinearWrapDesc}
    };
    PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
    PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    // 5. Create Camera Constant Buffer[cite: 1]
    BufferDesc CBDesc;
    CBDesc.Name = "Camera Constant Buffer";
    CBDesc.Size = sizeof(float4x4);
    CBDesc.Usage = USAGE_DYNAMIC;
    CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(CBDesc, nullptr, &m_pCameraCB);

    BufferDesc MatCBDesc;
    MatCBDesc.Name = "Material Constant Buffer";
    MatCBDesc.Size = sizeof(float4);
    MatCBDesc.Usage = USAGE_DYNAMIC;
    MatCBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    MatCBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    pDevice->CreateBuffer(MatCBDesc, nullptr, &m_pMaterialCB);

    // 6. Create Shader Resource Binding
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);
    if (auto* pCameraConstantsVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) {
        pCameraConstantsVar->Set(m_pCameraCB);
    }

    if (auto* pMatConstantsVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "MaterialConstants")) {
        pMatConstantsVar->Set(m_pMaterialCB);
    }
}

void Diligent::TexturedPipeline::StartFrameRender(IDeviceContext* pContext, CameraObj camera)
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

    //glm::mat4 mvp = transmat * view * proj;

    // Update the Constant Buffer[cite: 1]
    {
        MapHelper<glm::mat4> CBData(pContext, m_pCameraCB, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBData = mvp_t;
        //*CBData = mvp;
    }

    pContext->SetPipelineState(m_pPSO);

    // Note: We remove CommitShaderResources from here because we need to commit 
    // dynamically inside RenderModel after binding the submesh's specific texture.
}

void Diligent::TexturedPipeline::RenderModel(IDeviceContext* pContext, Model* model)
{
    // Bind vertex and index buffers[cite: 1]
    IBuffer* pBuffs[] = { model->pVertexBuffer };
    pContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);
    pContext->SetIndexBuffer(model->pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    for (const auto& submesh : model->SubMeshes) {

        // Bind the specific material texture for this submesh[cite: 6]
        if (submesh.MaterialIndex < model->Materials.size()) {
            // 1. Bind Texture (Will bind actual texture OR default white texture)
            if (auto pTextureView = model->Materials[submesh.MaterialIndex]) {
                m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_Texture")->Set(pTextureView);
            }

            // 2. Map and update the solid color for this specific material
            {
                MapHelper<float4> CBData(pContext, m_pMaterialCB, MAP_WRITE, MAP_FLAG_DISCARD);
                *CBData = model->MaterialColors[submesh.MaterialIndex];
            }
        }

        // Commit resources right before the draw call so the new texture is active
        pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        // --- Draw Call ---[cite: 1]
        DrawIndexedAttribs DrawAttrs;
        DrawAttrs.IndexType = VT_UINT32;
        DrawAttrs.NumIndices = submesh.IndexCount;
        DrawAttrs.FirstIndexLocation = submesh.IndexOffset;
        DrawAttrs.BaseVertex = submesh.BaseVertex;
        DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;

        pContext->DrawIndexed(DrawAttrs);
    }
}