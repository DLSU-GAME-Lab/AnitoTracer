#pragma once
#include "DeferredPipeline.hpp"

namespace Diligent {

    struct EditorConstants {
        glm::vec4 WireframeColor;
    };

    class EditorPipeline : public DeferredPipeline {
    public:
        void InitializePipeline(IRenderDevice* pDevice, ISwapChain* pSwapChain) override;

        // Overriding RenderModels to handle multi-pass drawing for editor overlays
        void RenderModels(IDeviceContext* pContext, RenderData renderData, bool renderOpaque = true) override;

        void SetWireframeMode(bool enable) { m_RenderWireframe = enable; }
        bool IsWireframeMode() const { return m_RenderWireframe; }

        void SetSurfaceMode(bool enable) { m_RenderSurfaces = enable; }
        bool IsSurfaceMode() const { return m_RenderSurfaces; }

        void SetWireframeColor(const glm::vec4& color) { m_WireframeColor = color; }
        glm::vec4 GetWireframeColor() const { return m_WireframeColor; }

    private:
        void InitializeWireframePSO(IRenderDevice* pDevice);
        void CreateEditorCB(IRenderDevice* pDevice);

        RefCntAutoPtr<IPipelineState> m_pWireframePSO;
        RefCntAutoPtr<IShaderResourceBinding> m_pWireframeSRB;
        RefCntAutoPtr<IBuffer> m_pEditorCB;

        bool m_RenderSurfaces = false;
        bool m_RenderWireframe = true;

        glm::vec4 m_WireframeColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    };
}