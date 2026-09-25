#include "EditorPanel.hpp"
#include <iostream>

#include "Event/EventSystem.hpp"
#include "RendererEvents.hpp"

namespace Diligent {
    EditorPanel::EditorPanel(const std::string& name, SRVGetter srvGetter)
        : ViewportPanel(name, std::move(srvGetter), true) // true for gizmos
    {
        // Require a menu bar for this panel
        m_WindowFlags |= ImGuiWindowFlags_MenuBar;
        m_barColor = ImVec4(0.15f, 0.20f, 0.30f, 1.0f);
    }

    void EditorPanel::DrawTopBar() {
        // Push a custom color for this menu bar (Dark Blue)
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, m_barColor);

        if (ImGui::BeginMenuBar()) {
            // Map the selected integer to a string label
            const char* rendererNames[] = { "Editor", "Game" };
            std::string menuLabel = "Renderer: " + std::string(rendererNames[m_SelectedRenderer]);

            // Use the dynamic label for the dropdown title
            if (ImGui::BeginMenu(menuLabel.c_str())) {
                if (ImGui::MenuItem("Editor", nullptr, m_SelectedRenderer == 0)) {
                    m_SelectedRenderer = 0;
                    std::cout << "[Debug] Selected Renderer: Editor\n";
                }
                if (ImGui::MenuItem("Game", nullptr, m_SelectedRenderer == 1)) {
                    m_SelectedRenderer = 1;
                    std::cout << "[Debug] Selected Renderer: Game\n";
                }
                ImGui::EndMenu();
            }

            // Our shiny new View Options dropdown!
            bool optionsChanged = false;
            if (ImGui::BeginMenu("View Options")) {
                if (ImGui::Checkbox("Show Surfaces", &m_ShowSurfaces)) optionsChanged = true;
                if (ImGui::Checkbox("Show Wireframe", &m_ShowWireframe)) optionsChanged = true;

                // ColorEdit4 gives Master a beautiful color picker with an alpha channel!
                if (ImGui::ColorEdit4("Wireframe Color", m_WireframeColor)) optionsChanged = true;

                ImGui::EndMenu();
            }

            // Dispatch the payload directly to the Manager if anything was touched
            if (optionsChanged) {
                gbe::EventSystem::DispatchTo(
                    EVENT_UPDATE_EDITOR_OPTIONS,
                    std::make_unique<UpdateEditorOptionsArgs>(
                        m_ShowWireframe,
                        m_ShowSurfaces,
                        glm::vec4(m_WireframeColor[0], m_WireframeColor[1], m_WireframeColor[2], m_WireframeColor[3])
                    )
                );
            }

            ImGui::EndMenuBar();
        }

        // Always pop style colors you push so they don't bleed into other windows
        ImGui::PopStyleColor();
    }
}