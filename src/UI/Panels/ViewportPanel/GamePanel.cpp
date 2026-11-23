#include "GamePanel.hpp"
#include "GUIManager.hpp"
#include <iostream>

namespace Diligent {
    GamePanel::GamePanel(const std::string& name, SRVGetter srvGetter)
        : ViewportPanel(name, std::move(srvGetter), false) // false for no gizmos
    {
        // Require a menu bar for this panel
        m_WindowFlags |= ImGuiWindowFlags_MenuBar;
        m_barColor = ImVec4(0.15f, 0.25f, 0.15f, 1.0f);
    }

    void GamePanel::OnBeforeBegin() {
        if (GUIManager::GetInstance().ConsumeGameViewportFocusRequest()) {
            ImGui::SetWindowFocus(m_Name.c_str());
        }
    }

    void GamePanel::OnViewportDrawn(ImVec2 pos, ImVec2 size, bool hovered, bool focused) {
        GUIManager::GetInstance().SetGameViewportInfo(pos, size, hovered, focused);
    }

    void GamePanel::DrawTopBar() {
        // Push a custom color for this menu bar (Dark Green)
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, m_barColor);

        if (ImGui::BeginMenuBar()) {
            DrawResolutionMenu();
            ImGui::EndMenuBar();
        }

        // Pop the color so the rest of the UI remains normal
        ImGui::PopStyleColor();
    }
}