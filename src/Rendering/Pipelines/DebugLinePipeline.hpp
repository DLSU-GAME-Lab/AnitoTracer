#pragma once

#include "BasePipeline.hpp"
#include "Utils/DynamicVertexBuffer.hpp"
#include <vector>

namespace Diligent {

    /// Minimal unlit line-list pipeline for Jolt debug visualization.
    /// Vertex buffer is re-uploaded every frame from whatever lines
    /// JoltDebugRenderer collected that frame.
    class DebugLinePipeline : public BasePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;

        // Uploads the given line vertices and issues one Draw call.
        // Call after StartFrameRender (camera CB must already be set for this frame).
        void RenderLines(IDeviceContext* pContext, const std::vector<DebugLineVertex>& lines);

    private:
        DynamicVertexBuffer m_Vertices{ "Debug Line Vertex Buffer", sizeof(DebugLineVertex), 4096, static_cast<Uint32>(kMaxDebugLineVertices) };
    };

}