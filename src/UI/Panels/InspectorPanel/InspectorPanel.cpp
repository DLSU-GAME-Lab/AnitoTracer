#include "InspectorPanel.hpp"
#include "TypeRegistry.hpp"
#include "SerializedData.hpp"

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
                    // Fall back to a default name if m_name was not populated during serialization
                    std::string compName = component->GetName();
                    if (compName.empty()) {
                        compName = "Unnamed Component";
                    }

                    // Append unique pointer address (##) to ensure non-empty, unique ImGui IDs
                    std::string headerLabel = compName + "##" + std::to_string(reinterpret_cast<uintptr_t>(component.get()));

                    if (ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                        for (auto* property : component->properties) {
                            if (property->m_id == "m_name") continue;

                            if (!property->DrawInspector()) {
                                std::string fallBackTxt = "No UI for " + property->m_display_name;
                                ImGui::Text("%s", fallBackTxt.c_str());
                            }
                        }
                    }
                    ImGui::Spacing();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // --- ADD COMPONENT BUTTON & POPUP ---
            if (ImGui::Button("Add Component", ImVec2(-1, 0))) {
                ImGui::OpenPopup("AddComponentPopup");
            }

            if (ImGui::BeginPopup("AddComponentPopup")) {
                for (const auto& entry : gbe::TypeRegistry::GetEntries()) {
                    std::string label = entry.name;
                    if (label.rfind("class ", 0) == 0) label.erase(0, 6);
                    if (label.rfind("struct ", 0) == 0) label.erase(0, 7);

                    // Ensure popup labels are non-empty and uniquely identified
                    std::string popupItemLabel = (label.empty() ? "Unknown Type" : label) + "##" + entry.name;

                    if (ImGui::Selectable(popupItemLabel.c_str())) {
                        gbe::SerializedData emptyData;
                        gbe::ISerializable* rawInstance = gbe::TypeRegistry::Instantiate(entry.name, emptyData);

                        if (auto* newComponent = dynamic_cast<ComponentBase*>(rawInstance)) {
                            newComponent->SetOwner(selected);
                            selected.GetPtr()->AddComponent(std::unique_ptr<ComponentBase>(newComponent));
                        }
                        else {
                            delete rawInstance;
                        }
                    }
                }
                ImGui::EndPopup();
            }
        }
        else
        {
            ImGui::Text("No object selected.");
        }
    }
    ImGui::End();
}