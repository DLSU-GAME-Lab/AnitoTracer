#pragma once
#include "ViewportPanel.hpp"

namespace Diligent {
    class GamePanel : public ViewportPanel {
    public:
        GamePanel(const std::string& name, SRVGetter srvGetter);
    protected:
        void DrawTopBar() override;
    private:
        int m_SelectedResolution = 1; // Default to 1080p
    };
}