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
                if (ImGui::MenuItem("Add Direction Light"))
                {
                    ObjectFactory::GetInstance().CreateDirectionalLightObject("Direction Light");
                }
                if (ImGui::MenuItem("Load Model"))
                {
                    // Use the static method from the new FileDialog class
                    std::string filepath = FileDialog::OpenModelFile();

                    if (!filepath.empty())
                    {
                        std::cout << "Successfully selected model: " << filepath << std::endl;
                        ObjectFactory::GetInstance().CreateModelObject("Loaded Model", filepath);
                    }
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