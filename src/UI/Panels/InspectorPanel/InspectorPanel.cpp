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
            // --- Gizmo Control Toolbar ---
            auto& gizmoDrawer = GUIManager::GetInstance().GetGizmoDrawer();
            ImGuizmo::OPERATION currentOp = gizmoDrawer.GetOperation();
            ImGuizmo::MODE currentMode = gizmoDrawer.GetMode();

            // Operation Buttons (Translate, Rotate, Scale)
            if (ImGui::RadioButton("Translate", currentOp == ImGuizmo::TRANSLATE))
                gizmoDrawer.SetOperation(ImGuizmo::TRANSLATE);

            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate", currentOp == ImGuizmo::ROTATE))
                gizmoDrawer.SetOperation(ImGuizmo::ROTATE);

            ImGui::SameLine();
            if (ImGui::RadioButton("Scale", currentOp == ImGuizmo::SCALE))
                gizmoDrawer.SetOperation(ImGuizmo::SCALE);

            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();

            // Toggle Local / World Space
            bool isLocal = (currentMode == ImGuizmo::LOCAL);
            if (ImGui::Checkbox("Local", &isLocal))
            {
                gizmoDrawer.SetMode(isLocal ? ImGuizmo::LOCAL : ImGuizmo::WORLD);
            }

            ImGui::Separator();
            ImGui::Spacing();

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
                    // Scope all widgets within this component to its memory address
                    ImGui::PushID(component.get());

                    InspectorRegistry::GetInstance().DrawComponent(component.get());

                    ImGui::SameLine();
                    if (ImGui::SmallButton("Remove")) {
                        selected.GetPtr()->RemoveComponent(component.get());
                        ImGui::PopID();
                        break;
                    }

                    ImGui::PopID(); // Pop ID scope after drawing component
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