#include "BasePipeline.hpp"

void Diligent::BasePipeline::StartFrameRender(IDeviceContext* pContext, RenderData renderData)
{
    //OLD IMPLEM FOR REF
    //glm::mat4 transmat = renderData.Models[0].WorldTransform;

    //glm::mat4 view = renderData.ViewMatrix;
    //glm::mat4 proj = renderData.ProjectionMatrix;

    //glm::mat4 mvp = proj * view * transmat;
    //glm::mat4 mvp_t = glm::transpose(mvp);

    //// Forces out of scope cleanup
    //{
    //    MapHelper<glm::mat4> CBData(pContext, m_pCameraCB, MAP_WRITE, MAP_FLAG_DISCARD);
    //    *CBData = mvp_t;
    //}

    //pContext->SetPipelineState(m_pPSO);

    glm::mat4 view = renderData.ViewMatrix;
    glm::mat4 proj = renderData.ProjectionMatrix;

    // Forces out of scope cleanup
    {
        MapHelper<CameraConstants> CBData(pContext, m_pCameraCB, MAP_WRITE, MAP_FLAG_DISCARD);
        CBData->View = glm::transpose(view);
        CBData->Proj = glm::transpose(proj);
    }

    pContext->SetPipelineState(m_pPSO);
}

void Diligent::BasePipeline::RenderModels(IDeviceContext* pContext, RenderData renderData)
{
    for (int i = 0; i < renderData.Models.size(); i++) {
        RenderModel(pContext, renderData.Models[i]);
    }
}

void Diligent::BasePipeline::CreateCameraConstantBuffer(IRenderDevice* pDevice)
{
    BufferDesc CBDesc;
    CBDesc.Name = "Camera Constant Buffer";
    CBDesc.Size = sizeof(float4x4) * 2; // Updated to hold View and Proj matrices
    CBDesc.Usage = USAGE_DYNAMIC;
    CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    pDevice->CreateBuffer(CBDesc, nullptr, &m_pCameraCB);
}

void Diligent::BasePipeline::CreateModelConstantBuffer(IRenderDevice* pDevice)
{
    BufferDesc CBDesc;
    CBDesc.Name = "Model Constant Buffer";
    CBDesc.Size = sizeof(float4x4); // Size of the model matrix
    CBDesc.Usage = USAGE_DYNAMIC;
    CBDesc.BindFlags = BIND_UNIFORM_BUFFER;
    CBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;

    pDevice->CreateBuffer(CBDesc, nullptr, &m_pModelCB);
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
