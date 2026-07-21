#include "InspectorPanel.hpp"

void Diligent::InspectorPanel::Draw()
{
    if (!m_IsVisible) return;

    if (ImGui::Begin(m_Name.c_str(), &m_IsVisible))
    {
        HierarchyObject* selected = m_HierarchyPanel->GetSelectedObject();

        if (selected)
        {
            // Display the object's name
            ImGui::TextDisabled("Name:");
            ImGui::SameLine();
            ImGui::Text("%s", selected->GetName().c_str());

            ImGui::Separator();
            ImGui::Spacing();

            // Loop through all attached components and render their modular UI
            for (const auto& component : selected->GetComponents())
            {
                if (component)
                {
                    InspectorRegistry::GetInstance().DrawComponent(component.get());
                    ImGui::Spacing();
                }
            }
        }
        else
        {
            ImGui::Text("No object selected.");
        }
    }
    ImGui::End();
}
