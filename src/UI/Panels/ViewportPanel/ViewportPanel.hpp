#pragma once
#include "Panels/BasePanel.hpp"
#include "Graphics/GraphicsEngine/interface/TextureView.h"
#include <functional>
#include <string>

namespace Diligent {
    class ViewportPanel : public BasePanel {
    public:
        using SRVGetter = std::function<ITextureView* ()>;

        ViewportPanel(const std::string& name, SRVGetter srvGetter, bool drawGizmos = false);
        ~ViewportPanel() override = default;

        void Draw() override;
    protected:
        //For menu bar
        virtual void DrawTopBar() {}

        ImVec4 m_barColor = ImVec4(1.0f, 0.25f, 0.15f, 1.0f);

        ImGuiWindowFlags m_WindowFlags = 0;
        SRVGetter m_GetSRV;
        bool m_DrawGizmos;
    };
}