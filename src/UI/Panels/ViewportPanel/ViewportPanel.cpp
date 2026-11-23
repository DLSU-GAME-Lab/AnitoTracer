#include "ViewportPanel.hpp"
#include <imgui.h>
#include <cmath>
#include "GUIManager.hpp"
#include "../../UserSettings.hpp"
#include "../../Objects/HierarchyManager.hpp"
#include "../../Objects/Components/EditorCamera.hpp"
#include "../../ObjectSystems/Event/Types/OnGUI_Editor.hpp"

namespace Diligent {
    ViewportPanel::ViewportPanel(const std::string& name, SRVGetter srvGetter, bool drawGizmos)
        : BasePanel(name), m_GetSRV(std::move(srvGetter)), m_DrawGizmos(drawGizmos) {}

    void ViewportPanel::DrawResolutionMenu() {
        int& resolutionIndex = UserSettings::GetInstance().GetResolutionIndex();
        std::string menuLabel = "Resolution: " + std::string(UserSettings::GetInstance().GetResolution().Name);

        if (ImGui::BeginMenu(menuLabel.c_str())) {
            for (int i = 0; i < kRenderResolutionCount; ++i) {
                const RenderResolution& resolution = kRenderResolutions[i];
                const std::string size = std::to_string(resolution.Width) + "x" + std::to_string(resolution.Height);
                if (ImGui::MenuItem(resolution.Name, size.c_str(), resolutionIndex == i)) {
                    resolutionIndex = i;
                }
            }
            ImGui::EndMenu();
        }
    }

    void ViewportPanel::Draw() {
        if (!m_IsVisible) return;

        OnBeforeBegin();

        // Remove padding so the render target sits flush with the window borders
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        //Prevent jitters
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | m_WindowFlags;

        // Begin returns false when the window is collapsed or docked behind another tab
        m_IsShown = ImGui::Begin(m_Name.c_str(), &m_IsVisible, flags);
        if (m_IsShown) {
            DrawTopBar();

            const ImVec2 contentSize = ImGui::GetContentRegionAvail();
            const ImVec2 contentPos = ImGui::GetCursorScreenPos();

            // The image's rect in the panel. Picking, camera input and gizmos all map through it, so they stay aligned in either scale mode.
            ImVec2 viewportSize = contentSize;
            ImVec2 cursorPos = contentPos;

            ITextureView* pSRV = m_GetSRV ? m_GetSRV() : nullptr;
            if (pSRV && UserSettings::GetInstance().GetViewportScaleMode() == ViewportScaleMode::Letterbox
                && contentSize.x > 0.0f && contentSize.y > 0.0f) {
                const RenderResolution& resolution = UserSettings::GetInstance().GetResolution();
                const float targetAspect = static_cast<float>(resolution.Width) / static_cast<float>(resolution.Height);

                // Fit the resolution's shape inside the panel, centered; whole pixels keep the image sharp
                if (contentSize.x / contentSize.y > targetAspect) {
                    viewportSize.x = std::floor(contentSize.y * targetAspect);
                }
                else {
                    viewportSize.y = std::floor(contentSize.x / targetAspect);
                }
                cursorPos.x = std::floor(contentPos.x + (contentSize.x - viewportSize.x) * 0.5f);
                cursorPos.y = std::floor(contentPos.y + (contentSize.y - viewportSize.y) * 0.5f);

                // Bars
                ImGui::GetWindowDrawList()->AddRectFilled(contentPos,
                    ImVec2(contentPos.x + contentSize.x, contentPos.y + contentSize.y), IM_COL32(0, 0, 0, 255));
                ImGui::SetCursorScreenPos(cursorPos);
            }

            if (pSRV) {
                // Diligent accepts ITextureView* cast to ImTextureID
                ImGui::Image(reinterpret_cast<ImTextureID>(pSRV), viewportSize);

                const bool imageHovered = ImGui::IsItemHovered();
                const bool windowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
                OnViewportDrawn(cursorPos, viewportSize, imageHovered, windowFocused);

                if (m_DrawGizmos) {
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