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

            if (ImGui::BeginMenu("Assets"))
            {
                if (ImGui::MenuItem("Add Empty Object"))
                {
                    ObjectFactory::GetInstance().CreateRootObjectWithTransform("Empty Object");
                }

                if (ImGui::BeginMenu("Primmitives")) {
                    if (ImGui::MenuItem("Box")) {
                        ObjectFactory::GetInstance().CreateCubePrimitive("Box");
                    }
                    if (ImGui::MenuItem("Sphere")) {
                        ObjectFactory::GetInstance().CreateSpherePrimitive("Sphere");
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Lights")) {
                    if (ImGui::MenuItem("Direction")) {
                        ObjectFactory::GetInstance().CreateDirectionalLightObject("Direction Light");
                    }
                    if (ImGui::MenuItem("Point")) {
                        ObjectFactory::GetInstance().CreatePointLightObject("Point Light");
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Load Model"))
                {
                    // Placeholder for now
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