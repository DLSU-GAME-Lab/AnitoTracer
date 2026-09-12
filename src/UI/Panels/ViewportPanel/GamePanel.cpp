#include "GamePanel.hpp"
#include <iostream>

namespace Diligent {
    GamePanel::GamePanel(const std::string& name, SRVGetter srvGetter)
        : ViewportPanel(name, std::move(srvGetter), false) // false for no gizmos
    {
        // Require a menu bar for this panel
        m_WindowFlags |= ImGuiWindowFlags_MenuBar;
        m_barColor = ImVec4(0.15f, 0.25f, 0.15f, 1.0f);
    }

    void GamePanel::DrawTopBar() {
        // Push a custom color for this menu bar (Dark Green)
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, m_barColor);

        if (ImGui::BeginMenuBar()) {
            // Map the selected integer to a string label
            const char* resolutionNames[] = { "720p", "1080p", "1200p" };
            std::string menuLabel = "Resolution: " + std::string(resolutionNames[m_SelectedResolution]);

            // Use the dynamic label for the dropdown title
            if (ImGui::BeginMenu(menuLabel.c_str())) {
                if (ImGui::MenuItem("720p", nullptr, m_SelectedResolution == 0)) {
                    m_SelectedResolution = 0;
                    std::cout << "[Debug] Game Resolution changed to 720p\n";
                }
                if (ImGui::MenuItem("1080p", nullptr, m_SelectedResolution == 1)) {
                    m_SelectedResolution = 1;
                    std::cout << "[Debug] Game Resolution changed to 1080p\n";
                }
                if (ImGui::MenuItem("1200p", nullptr, m_SelectedResolution == 2)) {
                    m_SelectedResolution = 2;
                    std::cout << "[Debug] Game Resolution changed to 1200p\n";
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Pop the color so the rest of the UI remains normal
        ImGui::PopStyleColor();
    }
}