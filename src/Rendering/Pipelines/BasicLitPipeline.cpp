#include "BasicLitPipeline.hpp"

void Diligent::BasicLitPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    SetDevice(pDevice, pSwapChain);
    CreateCommonConstantBuffers();

    m_pPSO = CreateModelPSOBuilder("PBR Fallback (Non-RT) Rendering PSO", "fallbackLit_ps.hlsl")
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .Build(pDevice);
    m_pSRB = PipelineUtils::CreateSRB(m_pPSO);
    BindCommonBuffers(m_pSRB);
}
