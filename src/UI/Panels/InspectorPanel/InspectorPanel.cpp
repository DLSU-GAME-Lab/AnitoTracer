#include "InspectorPanel.hpp"

void Diligent::InspectorPanel::Draw()
{
    if (!m_IsVisible) return;

    if (ImGui::Begin(m_Name.c_str(), &m_IsVisible))
    {
        HierarchyObject::Ref selected = m_HierarchyPanel->GetSelectedObject();

        if (selected.GetPtr())
        {
            // Display the object's name
            ImGui::TextDisabled("Name:");
            ImGui::SameLine();
            ImGui::Text("%s", selected.GetPtr()->GetName().c_str());

            ImGui::Separator();
            ImGui::Spacing();

            // Loop through all attached components and render their modular UI
            for (const auto& component : selected.GetPtr()->GetComponents())
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
