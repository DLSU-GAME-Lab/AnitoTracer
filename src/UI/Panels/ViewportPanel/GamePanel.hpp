#pragma once
#include "ViewportPanel.hpp"

namespace Diligent {
    class GamePanel : public ViewportPanel {
    public:
        GamePanel(const std::string& name, SRVGetter srvGetter);
    protected:
        void DrawTopBar() override;
        void OnBeforeBegin() override;
        void OnViewportDrawn(ImVec2 pos, ImVec2 size, bool hovered, bool focused) override;
    };
}