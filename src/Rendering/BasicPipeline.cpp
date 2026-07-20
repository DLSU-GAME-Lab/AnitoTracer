#include "BasicPipeline.hpp"

void Diligent::BasicPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    // 1. Basic Pipeline properties
    PSODesc.Name = "Basic Rendering PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    GraphicsPipeline.NumRenderTargets = 1;
    GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = True;

    // 2. Input Layout (Must match your VSInput struct exactly)
    LayoutElement LayoutElems[] =
    {
        // Attribute 0: float3 Pos
        LayoutElement{0, 0, 3, VT_FLOAT32, False},
        // Attribute 1: float4 Color
        LayoutElement{1, 0, 4, VT_FLOAT32, False}
    };
    GraphicsPipeline.InputLayout.LayoutElements = LayoutElems;
    GraphicsPipeline.InputLayout.NumElements = _countof(LayoutElems);

    auto pVS = ShaderManager::GetInstance().GetShader("basic.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("basic.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");

    // 3. Assign Shaders
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    // 4. Define Resource Layout (Telling the pipeline about the constant buffer)
    ShaderResourceVariableDesc Variables[] =
    {
        // DYNAMIC type allows us to update the buffer every frame or per-object
        {SHADER_TYPE_VERTEX, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    PSODesc.ResourceLayout.Variables = Variables;
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);

    // Create the Pipeline State
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    // 5. Create the Constant Buffer for the camera
    BufferDesc CBDesc;
    CBDesc.Name = "Camera Constant Buffer";
    CBDesc.Size = sizeof(float4x4);
    CBDesc.Usage = USAGE_DYNAMIC;            // Fast CPU writes
    CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    pDevice->CreateBuffer(CBDesc, nullptr, &m_pCameraCB);

    // 6. Create the Shader Resource Binding (SRB) and bind the buffer
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

    // 3. Safely look up and bind the variable
    if (auto* pCameraConstantsVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants"))
    {
        pCameraConstantsVar->Set(m_pCameraCB);
    }
    else
    {
        std::cout << "Variable not found" << std::endl;
        // If you hit this, the variable was either spelled wrong in C++, 
        // mapped to the wrong shader stage, or optimized out by the HLSL compiler.
        // Log a warning here so you don't crash!
    }
}

void Diligent::BasicPipeline::StartFrameRender(IDeviceContext* pContext, CameraObj camera)
{
    glm::mat4 transmat = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
    transmat = glm::scale(transmat, glm::vec3(10.f, 10.f, 10.f));

    auto rawPos = camera.GetPosition();
    glm::vec3 cameraPos = glm::vec3(rawPos.x, rawPos.y, rawPos.z);

    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0), glm::vec3(0, 1, 0));

    float aspectRatio = static_cast<float>(pSwapChain->GetDesc().Width) / pSwapChain->GetDesc().Height;
    glm::mat4 proj = glm::perspective(glm::radians(45.f), aspectRatio, 0.1f, 100.0f);

    glm::mat4 mvp = proj * view * transmat;
    glm::mat4 mvp_t = glm::transpose(mvp);

    // 6. Update the Constant Buffer
    {
        MapHelper<glm::mat4> CBData(pContext, m_pCameraCB, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBData = mvp_t;
    }

    pContext->SetPipelineState(m_pPSO);
    pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}
