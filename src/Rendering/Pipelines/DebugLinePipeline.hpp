#pragma once

#include "BasePipeline.hpp"
#include <vector>

namespace Diligent {

    /// Minimal unlit line-list pipeline for Jolt debug visualization.
    /// Vertex buffer is re-uploaded every frame from whatever lines
    /// JoltDebugRenderer collected that frame.
    class DebugLinePipeline : public BasePipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* _pSwapChain) override;

        // Uploads the given line vertices and issues one Draw call.
        // Call after StartFrameRender (camera CB must already be set for this frame).
        void RenderLines(IDeviceContext* pContext, const std::vector<DebugLineVertex>& lines);

    private:
        RefCntAutoPtr<IBuffer> m_pVertexBuffer;
        Uint32 m_VertexBufferCapacity = 0;

        void EnsureVertexBufferCapacity(IRenderDevice* pDevice, Uint32 requiredVertexCount);
    };

}