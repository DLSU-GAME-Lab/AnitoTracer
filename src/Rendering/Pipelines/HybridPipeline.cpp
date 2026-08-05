#include "HybridPipeline.hpp"
#include "../RenderData.hpp"
#include "../../UserSettings.hpp"

void Diligent::HybridPipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    pSwapChain = _pSwapChain;
    m_pDevice = pDevice;

    Uint8 sampleCount = UserSettings::GetInstance().GetEnableMSAA() ? 4 : 1;

    if (!m_pTLAS) {
        InitializeTLAS(pDevice);
    }

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "PBR Rendering PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);
    GraphicsPipeline.SmplDesc.Count = sampleCount;

    std::vector<LayoutElement> std_layout = VertexLayouts::GetStandardLayout();
    GraphicsPipeline.InputLayout.LayoutElements = std_layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(std_layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("hybrid_vs.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("hybrid_ps.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    std::vector<ShaderResourceVariableDesc> Variables = GetStandardShaderVariables();
    Variables.push_back({ SHADER_TYPE_PIXEL, "g_TLAS", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }); // Hybrid-only requirement

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
    if (auto* pVar = m_pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_TLAS"))           pVar->Set(m_pTLAS);
}

void Diligent::HybridPipeline::StartFrameRender(IDeviceContext* pContext, RenderData renderData)
{
    BasePipeline::StartFrameRender(pContext, renderData);
    BuildSceneTLAS(pContext, renderData);
}

void Diligent::HybridPipeline::InitializeTLAS(IRenderDevice* pDevice, Uint32 maxInstances)
{
    TopLevelASDesc TLASDesc;
    TLASDesc.Name = "Scene TLAS";
    TLASDesc.MaxInstanceCount = maxInstances;
    TLASDesc.Flags = RAYTRACING_BUILD_AS_PREFER_FAST_TRACE | RAYTRACING_BUILD_AS_ALLOW_UPDATE;

    pDevice->CreateTLAS(TLASDesc, &m_pTLAS);

    ScratchBufferSizes ScratchSizes = m_pTLAS->GetScratchBufferSizes();

    BufferDesc ScratchBuffDesc;
    ScratchBuffDesc.Name = "TLAS Build Scratch Buffer";
    ScratchBuffDesc.Size = ScratchSizes.Build;
    ScratchBuffDesc.Usage = USAGE_DEFAULT;
    ScratchBuffDesc.BindFlags = BIND_RAY_TRACING;

    pDevice->CreateBuffer(ScratchBuffDesc, nullptr, &m_pTLASScratchBuffer);
}

void Diligent::HybridPipeline::BuildSceneTLAS(IDeviceContext* pContext, const RenderData& renderData)
{
    if (renderData.Models.empty() || !m_pTLAS) return;

    std::vector<TLASBuildInstanceData> Instances;
    Instances.reserve(renderData.Models.size());

    for (size_t i = 0; i < renderData.Models.size(); ++i) {
        const auto& modelInstance = renderData.Models[i];
        if (!modelInstance.ModelData->pBLAS) continue;

        TLASBuildInstanceData tlasInst{};
        tlasInst.InstanceName = "ModelInstance " + i;
        tlasInst.pBLAS = modelInstance.ModelData->pBLAS;
        tlasInst.CustomId = static_cast<Uint32>(i);
        tlasInst.Flags = RAYTRACING_INSTANCE_NONE;
        tlasInst.Mask = 0xFF;

        const glm::mat4& world = modelInstance.WorldTransform;
        float* pTransformData = reinterpret_cast<float*>(&tlasInst.Transform);

        pTransformData[0] = world[0][0]; pTransformData[1] = world[1][0];
        pTransformData[2] = world[2][0]; pTransformData[3] = world[3][0];

        pTransformData[4] = world[0][1]; pTransformData[5] = world[1][1];
        pTransformData[6] = world[2][1]; pTransformData[7] = world[3][1];

        pTransformData[8] = world[0][2]; pTransformData[9] = world[1][2];
        pTransformData[10] = world[2][2]; pTransformData[11] = world[3][2];

        Instances.push_back(tlasInst);
    }

    if (Instances.empty()) return;

    Uint32 requiredInstanceBufferSize = static_cast<Uint32>(Instances.size() * sizeof(TLASBuildInstanceData));

    if (!m_pTLASInstanceBuffer || m_pTLASInstanceBuffer->GetDesc().Size < requiredInstanceBufferSize) {
        BufferDesc InstBuffDesc;
        InstBuffDesc.Name = "TLAS Instance Buffer";
        InstBuffDesc.Size = requiredInstanceBufferSize * 2;

        InstBuffDesc.Usage = USAGE_DEFAULT;
        InstBuffDesc.BindFlags = BIND_RAY_TRACING;
        InstBuffDesc.CPUAccessFlags = CPU_ACCESS_NONE;

        m_pDevice->CreateBuffer(InstBuffDesc, nullptr, &m_pTLASInstanceBuffer);
    }

    BuildTLASAttribs BuildAttribs;
    BuildAttribs.pTLAS = m_pTLAS;
    BuildAttribs.pInstances = Instances.data();
    BuildAttribs.InstanceCount = static_cast<Uint32>(Instances.size());
    BuildAttribs.pInstanceBuffer = m_pTLASInstanceBuffer;
    BuildAttribs.pScratchBuffer = m_pTLASScratchBuffer;

    BuildAttribs.TLASTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.BLASTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.InstanceBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    BuildAttribs.ScratchBufferTransitionMode = RESOURCE_STATE_TRANSITION_MODE_TRANSITION;

    pContext->BuildTLAS(BuildAttribs);
}