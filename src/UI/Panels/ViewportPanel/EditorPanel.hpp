#pragma once
#include "ViewportPanel.hpp"

namespace Diligent {
    class EditorPanel : public ViewportPanel {
    public:
        EditorPanel(const std::string& name, SRVGetter srvGetter);
    protected:
        void DrawTopBar() override;
    private:
        int m_SelectedRenderer = 0; // 0 = Editor, 1 = Game

        // UI State Variables for our Editor view options!
        bool m_ShowSurfaces = false;
        bool m_ShowWireframe = true;
        float m_WireframeColor[4] = { 0.0f, 1.0f, 0.0f, 1.0f }; // Default to green
    };
}