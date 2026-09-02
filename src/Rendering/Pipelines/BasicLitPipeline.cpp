#include "BasicLitPipeline.hpp"
#include "../RenderData.hpp"
#include "../../UserSettings.hpp"

void Diligent::BasicLitPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;
    m_pDevice = pDevice;

    Uint8 sampleCount = UserSettings::GetInstance().GetEnableMSAA() ? 4 : 1;

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "PBR Fallback (Non-RT) Rendering PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);
    GraphicsPipeline.SmplDesc.Count = sampleCount;

    std::vector<LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("main_vs.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("fallbackLit_ps.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
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

    if (!m_pCameraCB) CreateCameraConstantBuffer(pDevice);
    if (!m_pModelCB) CreateModelConstantBuffer(pDevice);

    // Utilize refactored buffer helper
    CreateCommonConstantBuffers(pDevice);

    m_pSRB.Release();
    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) pVar->Set(m_pCameraCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "ModelConstants"))  pVar->Set(m_pModelCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "LightConstants"))   pVar->Set(m_pLightCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "PBRMaterialConstants")) pVar->Set(m_pMaterialCB);
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "ShadowSettings"))   pVar->Set(m_pShadowCB);
}

void Diligent::BasicLitPipeline::StartFrameRender(IDeviceContext* pContext, RenderData renderData)
{
    BasePipeline::StartFrameRender(pContext, renderData);
}