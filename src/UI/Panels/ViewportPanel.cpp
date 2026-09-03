#include "ViewportPanel.hpp"
#include <imgui.h>
#include "GUIManager.hpp"
#include "../../Objects/HierarchyManager.hpp"
#include "../../Objects/Components/EditorCamera.hpp"

namespace Diligent {
    ViewportPanel::ViewportPanel(const std::string& name, SRVGetter srvGetter, bool drawGizmos)
        : BasePanel(name), m_GetSRV(std::move(srvGetter)), m_DrawGizmos(drawGizmos) {}

    void ViewportPanel::Draw() {
        if (!m_IsVisible) return;

        // Remove padding so the render target sits flush with the window borders
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        if (ImGui::Begin(m_Name.c_str(), &m_IsVisible)) {
            ImVec2 viewportSize = ImGui::GetContentRegionAvail();

            // We use the screen cursor pos to perfectly align ImGuizmo over the image
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();

            ITextureView* pSRV = m_GetSRV ? m_GetSRV() : nullptr;
            if (pSRV) {
                // Diligent accepts ITextureView* cast to ImTextureID
                ImGui::Image(reinterpret_cast<ImTextureID>(pSRV), viewportSize);
            }

            // Render Gizmos directly on top of this panel's image
            if (m_DrawGizmos && pSRV) {
                auto editorCam = gbe::IInstanceManager<EditorCamera>::getOldest();
                if (editorCam) {
                    GUIManager::GetInstance().DrawGizmos(
                        editorCam,
                        cursorPos.x, cursorPos.y,
                        viewportSize.x, viewportSize.y
                    );
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}