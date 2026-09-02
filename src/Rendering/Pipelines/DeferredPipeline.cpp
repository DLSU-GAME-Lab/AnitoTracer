#include "DeferredPipeline.hpp"
#include "../RenderData.hpp"
#include "../../UserSettings.hpp"

void Diligent::DeferredPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;
    m_pDevice = pDevice;

    if (!m_pCameraCB) CreateCameraConstantBuffer(pDevice);
    if (!m_pModelCB) CreateModelConstantBuffer(pDevice);
    CreateCommonConstantBuffers(pDevice);

    const auto& SCDesc = pSwapChain->GetDesc();
    CreateGBuffers(pDevice, SCDesc.Width, SCDesc.Height);

    InitializeGBufferPSO(pDevice);
    InitializeLightingPSO(pDevice);
}

void Diligent::DeferredPipeline::InitializeGBufferPSO(IRenderDevice* pDevice)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Deferred G-Buffer PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);

    // G-Buffer uses 3 Render Targets as defined in the pixel shader
    GraphicsPipeline.NumRenderTargets = 3;
    GraphicsPipeline.RTVFormats[0] = TEX_FORMAT_RGBA8_UNORM;       // AlbedoMetallic
    GraphicsPipeline.RTVFormats[1] = TEX_FORMAT_RGBA16_FLOAT;      // NormalRoughness
    GraphicsPipeline.RTVFormats[2] = TEX_FORMAT_RGBA32_FLOAT;      // WorldPos
    GraphicsPipeline.DSVFormat = pSwapChain->GetDesc().DepthBufferFormat;

    std::vector<LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("main_vs.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("Deferred/gbuffer_ps.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    std::vector<ShaderResourceVariableDesc> Variables = GetStandardShaderVariables();
    PSODesc.ResourceLayout.Variables = Variables.data();
    PSODesc.ResourceLayout.NumVariables = static_cast<Uint32>(Variables.size());

    std::vector<ImmutableSamplerDesc> ImtblSamplers = GetStandardImmutableSamplers();
    PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers.data();
    PSODesc.ResourceLayout.NumImmutableSamplers = static_cast<Uint32>(ImtblSamplers.size());

    m_pPSO.Release();
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    m_pSRB.Release();
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) pVar->Set(m_pCameraCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ModelConstants"))  pVar->Set(m_pModelCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "PBRMaterialConstants")) pVar->Set(m_pMaterialCB);
}

void Diligent::DeferredPipeline::InitializeLightingPSO(IRenderDevice* pDevice)
{
    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Deferred Lighting PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    GraphicsPipeline.NumRenderTargets = 1;
    GraphicsPipeline.RTVFormats[0] = pSwapChain->GetDesc().ColorBufferFormat;
    GraphicsPipeline.DSVFormat = TEX_FORMAT_UNKNOWN; // No depth testing for lighting pass
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    GraphicsPipeline.DepthStencilDesc.DepthEnable = False;
    GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = False;

    auto pVS = ShaderManager::GetInstance().GetShader("Deferred/fullscreen_vs.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("Deferred/simple_deferred_light_ps.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    std::vector<ShaderResourceVariableDesc> Variables = {
        {SHADER_TYPE_PIXEL, "g_GBufferAlbedo", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_GBufferNormal", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "g_GBufferWorldPos", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {SHADER_TYPE_PIXEL, "LightConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {SHADER_TYPE_PIXEL, "ShadowSettings", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    PSODesc.ResourceLayout.Variables = Variables.data();
    PSODesc.ResourceLayout.NumVariables = static_cast<Uint32>(Variables.size());

    SamplerDesc SamLinearClampDesc;
    SamLinearClampDesc.MinFilter = FILTER_TYPE_LINEAR;
    SamLinearClampDesc.MagFilter = FILTER_TYPE_LINEAR;
    SamLinearClampDesc.MipFilter = FILTER_TYPE_LINEAR;
    SamLinearClampDesc.AddressU = TEXTURE_ADDRESS_CLAMP;
    SamLinearClampDesc.AddressV = TEXTURE_ADDRESS_CLAMP;
    SamLinearClampDesc.AddressW = TEXTURE_ADDRESS_CLAMP;

    ImmutableSamplerDesc ImtblSamplers[] = {
        {SHADER_TYPE_PIXEL, "g_GBuffer_sampler", SamLinearClampDesc}
    };
    PSODesc.ResourceLayout.ImmutableSamplers = ImtblSamplers;
    PSODesc.ResourceLayout.NumImmutableSamplers = _countof(ImtblSamplers);

    m_pLightingPSO.Release();
    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pLightingPSO);

    m_pLightingSRB.Release();
    m_pLightingPSO->CreateShaderResourceBinding(&m_pLightingSRB, true);

    m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBufferAlbedo")->Set(m_pGBufferAlbedo->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBufferNormal")->Set(m_pGBufferNormal->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBufferWorldPos")->Set(m_pGBufferWorldPos->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));

    if (auto* pVar = m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "LightConstants")) pVar->Set(m_pLightCB);
    if (auto* pVar = m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "ShadowSettings")) pVar->Set(m_pShadowCB);
}

void Diligent::DeferredPipeline::CreateGBuffers(IRenderDevice* pDevice, Uint32 Width, Uint32 Height)
{
    TextureDesc TexDesc;
    TexDesc.Type = RESOURCE_DIM_TEX_2D;
    TexDesc.Width = Width;
    TexDesc.Height = Height;
    TexDesc.BindFlags = BIND_RENDER_TARGET | BIND_SHADER_RESOURCE;
    TexDesc.ClearValue.Format = TexDesc.Format;
    TexDesc.ClearValue.Color[0] = 0.0f;
    TexDesc.ClearValue.Color[1] = 0.0f;
    TexDesc.ClearValue.Color[2] = 0.0f;
    TexDesc.ClearValue.Color[3] = 0.0f;

    TexDesc.Name = "G-Buffer AlbedoMetallic";
    TexDesc.Format = TEX_FORMAT_RGBA8_UNORM;
    pDevice->CreateTexture(TexDesc, nullptr, &m_pGBufferAlbedo);

    TexDesc.Name = "G-Buffer NormalRoughness";
    TexDesc.Format = TEX_FORMAT_RGBA16_FLOAT;
    pDevice->CreateTexture(TexDesc, nullptr, &m_pGBufferNormal);

    TexDesc.Name = "G-Buffer WorldPos";
    TexDesc.Format = TEX_FORMAT_RGBA32_FLOAT;
    pDevice->CreateTexture(TexDesc, nullptr, &m_pGBufferWorldPos);

    TexDesc.Name = "Depth Buffer";
    TexDesc.Format = pSwapChain->GetDesc().DepthBufferFormat;
    TexDesc.BindFlags = BIND_DEPTH_STENCIL;
    TexDesc.ClearValue.DepthStencil.Depth = 1.f;
    pDevice->CreateTexture(TexDesc, nullptr, &m_pDepthBuffer);
}

void Diligent::DeferredPipeline::OnWindowResize(IRenderDevice* pDevice, Uint32 Width, Uint32 Height)
{
    m_pGBufferAlbedo.Release();
    m_pGBufferNormal.Release();
    m_pGBufferWorldPos.Release();
    m_pDepthBuffer.Release();

    CreateGBuffers(pDevice, Width, Height);

    m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBufferAlbedo")->Set(m_pGBufferAlbedo->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBufferNormal")->Set(m_pGBufferNormal->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
    m_pLightingSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_GBufferWorldPos")->Set(m_pGBufferWorldPos->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE));
}

void Diligent::DeferredPipeline::StartFrameRender(IDeviceContext* pContext, RenderData renderData)
{
    glm::mat4 view = renderData.ViewMatrix;
    glm::mat4 proj = renderData.ProjectionMatrix;

    {
        MapHelper<CameraConstants> CBData(pContext, m_pCameraCB, MAP_WRITE, MAP_FLAG_DISCARD);
        CBData->View = glm::transpose(view);
        CBData->Proj = glm::transpose(proj);
    }

    ITextureView* pRTVs[] = {
        m_pGBufferAlbedo->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
        m_pGBufferNormal->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET),
        m_pGBufferWorldPos->GetDefaultView(TEXTURE_VIEW_RENDER_TARGET)
    };
    ITextureView* pDSV = m_pDepthBuffer->GetDefaultView(TEXTURE_VIEW_DEPTH_STENCIL);

    pContext->SetRenderTargets(_countof(pRTVs), pRTVs, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const float ClearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    for (int i = 0; i < _countof(pRTVs); ++i) {
        pContext->ClearRenderTarget(pRTVs[i], ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    }
    pContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.0f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(m_pPSO);
}

void Diligent::DeferredPipeline::RenderLightingPass(IDeviceContext* pContext)
{
    auto* pRTV = pSwapChain->GetCurrentBackBufferRTV();
    pContext->SetRenderTargets(1, &pRTV, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    const float ClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    //pContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    pContext->SetPipelineState(m_pLightingPSO);
    pContext->CommitShaderResources(m_pLightingSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = 3;
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    pContext->Draw(DrawAttrs);
}