#include "DebugLinePipeline.hpp"
#include "../../UserSettings.hpp"
#include "../RenderData.hpp"

void Diligent::DebugLinePipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain)
{
    m_pSRB.Release();
    m_pPSO.Release();
    m_pCameraCB.Release();
    m_pVertexBuffer.Release();
    m_VertexBufferCapacity = 0;

    pSwapChain = _pSwapChain;
    m_pDevice = pDevice;

    GraphicsPipelineStateCreateInfo PSOCreateInfo;
    PipelineStateDesc& PSODesc = PSOCreateInfo.PSODesc;
    GraphicsPipelineDesc& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;

    PSODesc.Name = "Debug Line PSO";
    PSODesc.PipelineType = PIPELINE_TYPE_GRAPHICS;

    SetupDefaultGraphicsPipeline(GraphicsPipeline);
    GraphicsPipeline.SmplDesc.Count = UserSettings::GetInstance().GetEnableMSAA() ? 4 : 1;
    GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_LINE_LIST;
    GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
    // Depth-test against existing geometry so lines occlude correctly
    // but don't write depth so overlapping debug lines don't look weird
    GraphicsPipeline.DepthStencilDesc.DepthEnable = True;
    GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = False;

    std::vector<LayoutElement> layout = VertexLayouts::GetDebugLineLayout();
    GraphicsPipeline.InputLayout.LayoutElements = layout.data();
    GraphicsPipeline.InputLayout.NumElements = static_cast<Uint32>(layout.size());

    auto pVS = ShaderManager::GetInstance().GetShader("debug_line.hlsl", Diligent::SHADER_TYPE_VERTEX, "main_vs");
    auto pPS = ShaderManager::GetInstance().GetShader("debug_line.hlsl", Diligent::SHADER_TYPE_PIXEL, "main_ps");
    if (!pVS || !pPS)
    {
        LOG_ERROR_AND_THROW("debug_line.hlsl failed to load (VS: ", pVS ? "ok" : "null",
            ", PS: ", pPS ? "ok" : "null", ")");
    }

    PSOCreateInfo.pVS = pVS;
    PSOCreateInfo.pPS = pPS;

    ShaderResourceVariableDesc Variables[] =
    {
        {SHADER_TYPE_VERTEX, "CameraConstants", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
    };
    PSODesc.ResourceLayout.Variables = Variables;
    PSODesc.ResourceLayout.NumVariables = _countof(Variables);

    pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);

    CreateCameraConstantBuffer(pDevice);

    m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);

    if (auto* pCameraConstantsVar = m_pSRB->GetVariableByName(SHADER_TYPE_VERTEX, "CameraConstants")) {
        pCameraConstantsVar->Set(m_pCameraCB);
    }
}

void Diligent::DebugLinePipeline::EnsureVertexBufferCapacity(IRenderDevice* pDevice, Uint32 requiredVertexCount)
{
    if (m_pVertexBuffer && requiredVertexCount <= m_VertexBufferCapacity) {
        return;
    }

    // Grow with headroom
    Uint32 newCapacity = std::max<Uint32>(requiredVertexCount, m_VertexBufferCapacity == 0 ? 4096 : m_VertexBufferCapacity * 2);
    newCapacity = std::min<Uint32>(newCapacity, static_cast<Uint32>(kMaxDebugLineVertices));

    BufferDesc VBDesc;
    VBDesc.Name = "Debug Line Vertex Buffer";
    VBDesc.Usage = USAGE_DYNAMIC;
    VBDesc.BindFlags = BIND_VERTEX_BUFFER;
    VBDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
    VBDesc.Size = newCapacity * sizeof(DebugLineVertex);

    m_pVertexBuffer.Release();
    pDevice->CreateBuffer(VBDesc, nullptr, &m_pVertexBuffer);
    m_VertexBufferCapacity = newCapacity;
}

void Diligent::DebugLinePipeline::RenderLines(IDeviceContext* pContext, const std::vector<DebugLineVertex>& lines)
{
    if (lines.empty() || !m_pDevice) {
        return;
    }

    if (lines.size() > kMaxDebugLineVertices) return;

    EnsureVertexBufferCapacity(m_pDevice, static_cast<Uint32>(lines.size()));

    {
        MapHelper<DebugLineVertex> VBData(pContext, m_pVertexBuffer, MAP_WRITE, MAP_FLAG_DISCARD);
        DebugLineVertex* pDst = VBData;
        if (!pDst) return;
        memcpy(pDst, lines.data(), lines.size() * sizeof(DebugLineVertex));
    }

    pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    IBuffer* pBuffs[] = { m_pVertexBuffer };
    pContext->SetVertexBuffers(0, 1, pBuffs, nullptr, RESOURCE_STATE_TRANSITION_MODE_TRANSITION, SET_VERTEX_BUFFERS_FLAG_RESET);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = static_cast<Uint32>(lines.size());
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    pContext->Draw(DrawAttrs);
}