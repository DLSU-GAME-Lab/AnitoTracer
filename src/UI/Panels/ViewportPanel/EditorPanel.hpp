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
    };
}