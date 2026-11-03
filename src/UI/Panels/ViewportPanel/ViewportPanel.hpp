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

        // False while the panel is closed, collapsed or an inactive dock tab, so its view doesn't need rendering
        bool IsViewportShown() const { return m_IsVisible && m_IsShown; }
    protected:
        //For menu bar
        virtual void DrawTopBar() {}

        // Called right before ImGui::Begin so subclasses can request window focus/etc.
        virtual void OnBeforeBegin() {}

        // Called every frame the viewport image is drawn, regardless of gizmo support.
        virtual void OnViewportDrawn(ImVec2 pos, ImVec2 size, bool hovered, bool focused) {}

        ImVec4 m_barColor = ImVec4(1.0f, 0.25f, 0.15f, 1.0f);

        ImGuiWindowFlags m_WindowFlags = 0;
        SRVGetter m_GetSRV;
        bool m_DrawGizmos;
        bool m_IsShown = false;
    };
}