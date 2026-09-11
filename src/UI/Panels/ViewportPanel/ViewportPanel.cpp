#include "ViewportPanel.hpp"
#include <imgui.h>
#include "GUIManager.hpp"
#include "../../Objects/HierarchyManager.hpp"
#include "../../Objects/Components/EditorCamera.hpp"
#include "../../ObjectSystems/Event/Types/OnGUI_Editor.hpp"

namespace Diligent {
    ViewportPanel::ViewportPanel(const std::string& name, SRVGetter srvGetter, bool drawGizmos)
        : BasePanel(name), m_GetSRV(std::move(srvGetter)), m_DrawGizmos(drawGizmos) {}

    void ViewportPanel::Draw() {
        if (!m_IsVisible) return;

        // Remove padding so the render target sits flush with the window borders
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        //Prevent jitters
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        if (ImGui::Begin(m_Name.c_str(), &m_IsVisible, flags)) {
            ImVec2 viewportSize = ImGui::GetContentRegionAvail();

            // We use the screen cursor pos to perfectly align ImGuizmo over the image
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();

            ITextureView* pSRV = m_GetSRV ? m_GetSRV() : nullptr;
            if (pSRV) {
                // Diligent accepts ITextureView* cast to ImTextureID
                ImGui::Image(reinterpret_cast<ImTextureID>(pSRV), viewportSize);

                if (m_DrawGizmos) {
                    const bool imageHovered = ImGui::IsItemHovered();
                    const bool windowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
                    GUIManager::GetInstance().SetEditorViewportInfo(cursorPos, viewportSize, imageHovered, windowFocused);
                }
            }

            // Render Gizmos directly on top of this panel's image
            if (m_DrawGizmos && pSRV) {
                const ImVec2 viewportMax(cursorPos.x + viewportSize.x, cursorPos.y + viewportSize.y);
                ImDrawList* windowDrawList = ImGui::GetWindowDrawList();
                ImDrawList* foregroundDrawList = ImGui::GetForegroundDrawList();
                ImDrawList* backgroundDrawList = ImGui::GetBackgroundDrawList();

                // Keep viewport overlays constrained to the viewport image bounds.
                windowDrawList->PushClipRect(cursorPos, viewportMax, true);
                foregroundDrawList->PushClipRect(cursorPos, viewportMax, true);
                backgroundDrawList->PushClipRect(cursorPos, viewportMax, true);

                // Run editor GUI events in the viewport scope so debug overlays line up with this panel.
                HierarchyManager::GetInstance().DispatchEvent<OnGUI_Editor>(ImGui::GetIO().DeltaTime);

                auto editorCam = gbe::IInstanceManager<EditorCamera>::getOldest();
                if (editorCam) {
                    GUIManager::GetInstance().DrawGizmos(
                        editorCam,
                        cursorPos.x, cursorPos.y,
                        viewportSize.x, viewportSize.y
                    );
                }

                backgroundDrawList->PopClipRect();
                foregroundDrawList->PopClipRect();
                windowDrawList->PopClipRect();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}