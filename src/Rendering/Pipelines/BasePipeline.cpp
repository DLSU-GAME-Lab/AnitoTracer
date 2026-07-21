#include "BasePipeline.hpp"

void Diligent::BasePipeline::StartFrameRender(IDeviceContext* pContext, RenderData renderData)
{
    glm::mat4 transmat = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
    float sc = 4.f;
    transmat = glm::scale(transmat, glm::vec3(sc, sc, sc));

    glm::mat4 view = renderData.ViewMatrix;
    glm::mat4 proj = renderData.ProjectionMatrix;

    glm::mat4 mvp = proj * view * transmat;
    glm::mat4 mvp_t = glm::transpose(mvp);

    // Forces out of scope cleanup
    {
        MapHelper<glm::mat4> CBData(pContext, m_pCameraCB, MAP_WRITE, MAP_FLAG_DISCARD);
        *CBData = mvp_t;
    }

    pContext->SetPipelineState(m_pPSO);
}

void Diligent::BasePipeline::CreateCameraConstantBuffer(IRenderDevice* pDevice)
{
    BufferDesc CBDesc;
    CBDesc.Name = "Camera Constant Buffer";
    CBDesc.Size = sizeof(float4x4);
    CBDesc.Usage = USAGE_DYNAMIC;            // Fast CPU writes
    CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    pDevice->CreateBuffer(CBDesc, nullptr, &m_pCameraCB);
}

void Diligent::BasePipeline::SetupDefaultGraphicsPipeline(GraphicsPipelineDesc& GraphicsPipeline)
{
    GraphicsPipeline.NumRenderTargets = 1;
    GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_BACK;
    GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_LESS_EQUAL;
}

SamplerDesc Diligent::BasePipeline::GetLinearWrapSamplerDesc()
{
    SamplerDesc SamLinearWrapDesc;
    SamLinearWrapDesc.MinFilter = FILTER_TYPE_LINEAR;
    SamLinearWrapDesc.MagFilter = FILTER_TYPE_LINEAR;
    SamLinearWrapDesc.MipFilter = FILTER_TYPE_LINEAR;
    SamLinearWrapDesc.AddressU = TEXTURE_ADDRESS_WRAP;
    SamLinearWrapDesc.AddressV = TEXTURE_ADDRESS_WRAP;
    SamLinearWrapDesc.AddressW = TEXTURE_ADDRESS_WRAP;

    return SamLinearWrapDesc;
}
