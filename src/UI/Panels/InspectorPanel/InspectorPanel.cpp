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
                    //Skip name and use it to collapse
                    if (ImGui::CollapsingHeader(component->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

                        // Loop through the public properties vector from ISerializable
                        for (auto* property : component->properties) {

                            //Skip name
                            if (property->m_id == "m_name") {
                                continue;
                            }

                            // Draw all other properties normally
                            property->DrawInspector();
                        }
                    }

                    /*InspectorRegistry::GetInstance().DrawComponent(component.get());*/
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
