#include "DebugLinePipeline.hpp"
#include "../RenderData.hpp"

void Diligent::DebugLinePipeline::InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain)
{
    m_Vertices.Release();

    SetDevice(pDevice, pSwapChain);
    CreateCameraConstantBuffer();

    // Depth-test against existing geometry so lines occlude correctly
    // but don't write depth so overlapping debug lines don't look weird
    BuildMainPass(PSOBuilder("Debug Line PSO")
        .Shaders("debug_line.hlsl", "debug_line.hlsl")
        .RenderTargets({ GetColorFormat() }, GetDepthFormat())
        .SampleCount(PipelineUtils::GetMSAASampleCount())
        .Topology(PRIMITIVE_TOPOLOGY_LINE_LIST)
        .Cull(CULL_MODE_NONE)
        .Depth(true, false)
        .InputLayout(VertexLayouts::GetDebugLineLayout())
        .Variable(SHADER_TYPE_VERTEX, "CameraConstants"));
}

void Diligent::DebugLinePipeline::RenderLines(IDeviceContext* pContext, const std::vector<DebugLineVertex>& lines)
{
    if (lines.empty() || !m_pDevice || !m_pSRB) {
        return;
    }

    // Nothing is drawn when there are more lines than the buffer may hold
    if (!m_Vertices.Upload(m_pDevice, pContext, lines.data(), static_cast<Uint32>(lines.size()))) return;

    pContext->CommitShaderResources(m_pSRB, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    m_Vertices.Bind(pContext);

    DrawAttribs DrawAttrs;
    DrawAttrs.NumVertices = static_cast<Uint32>(lines.size());
    DrawAttrs.Flags = DRAW_FLAG_VERIFY_ALL;
    pContext->Draw(DrawAttrs);
}
