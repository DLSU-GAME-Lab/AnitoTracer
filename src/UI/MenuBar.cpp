#include "MenuBar.hpp"

namespace Diligent {

    void MenuBar::Draw(bool& appRunning, const std::vector<std::unique_ptr<BasePanel>>& panels)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    appRunning = false;
                }
                ImGui::EndMenu();
            }

            // Dynamically populate the Windows menu based on registered panels
            if (ImGui::BeginMenu("Windows"))
            {
                for (const auto& panel : panels)
                {
                    ImGui::MenuItem(panel->GetName().c_str(), NULL, &panel->GetVisible());
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

}