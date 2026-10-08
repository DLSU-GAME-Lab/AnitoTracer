#include "InspectorPanel.hpp"
#include "TypeRegistry.hpp"
#include "SerializedData.hpp"
#include "../../../Objects/Components/EditorCamera.hpp"
#include "../../../Objects/Components/Physics/Collider.hpp"

#include "HierarchyFeatures/PrefabFeature.hpp"

#include <cctype>
#include <cstring>

void Diligent::InspectorPanel::Draw()
{
    if (!m_IsVisible) {
        if (m_WasAnyItemActive) {
            HierarchyManager::GetInstance().EndUndoableAction();
            m_WasAnyItemActive = false;
        }
        return;
    }

    if (ImGui::Begin(m_Name.c_str(), &m_IsVisible))
    {
        HierarchyObject::Ref selected = m_HierarchyPanel->GetSelectedObject();

        if (selected.GetPtr())
        {
            if (selected.GetPtr()->GetComponent<EditorCamera>() != nullptr)
            {
                ImGui::Text("Editor camera is protected and cannot be edited here.");
                ImGui::End();
                return;
            }

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

            // Rename selected object
            if (m_NameBufferObject != selected)
            {
                m_NameBufferObject = selected;
                std::strncpy(m_NameBuffer, selected.GetPtr()->GetName().c_str(), sizeof(m_NameBuffer) - 1);
                m_NameBuffer[sizeof(m_NameBuffer) - 1] = '\0';
            }

            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText("Name", m_NameBuffer, sizeof(m_NameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                selected.GetPtr()->SetName(m_NameBuffer);
            }

            if (ImGui::IsItemDeactivatedAfterEdit())
            {
                selected.GetPtr()->SetName(m_NameBuffer);
            }

            ImGui::Separator();
            ImGui::Spacing();

            if (selected.GetPtr()->IsPrefabInstance())
            {
                ImGui::TextWrapped("Prefab: %s", selected.GetPtr()->GetPrefabAssetPath().c_str());

                if (ImGui::Button("Apply to Prefab"))
                {
                    PrefabFeature::GetInstance().ApplyPrefabToAsset(selected);
                }

                ImGui::SameLine();
                if (ImGui::Button("Revert from Prefab"))
                {
                    PrefabFeature::GetInstance().RevertPrefabInstance(selected);
                }

                ImGui::SameLine();
                if (ImGui::Button("Unpack"))
                {
                    PrefabFeature::GetInstance().UnpackPrefabInstance(selected);
                }
            }
            else if (ImGui::Button("Create Prefab Asset"))
            {
                PrefabFeature::GetInstance().CreatePrefabAsset(selected);
            }

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
                m_ComponentSearchBuffer[0] = '\0';
                ImGui::OpenPopup("AddComponentPopup");
            }

            if (ImGui::BeginPopup("AddComponentPopup")) {
                ImGui::SetNextItemWidth(-1.0f);
                ImGui::InputTextWithHint("##AddComponentSearch", "Search components...", m_ComponentSearchBuffer, sizeof(m_ComponentSearchBuffer));

                const auto toLower = [](std::string value) {
                    for (char& character : value) {
                        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
                    }
                    return value;
                };
                const std::string searchFilter = toLower(m_ComponentSearchBuffer);

                for (const auto& entry : gbe::TypeRegistry::GetEntries()) {
                    std::string label = entry.name;
                    if (label.rfind("class ", 0) == 0) label.erase(0, 6);
                    if (label.rfind("struct ", 0) == 0) label.erase(0, 7);

                    if (!searchFilter.empty() && toLower(label).find(searchFilter) == std::string::npos) {
                        continue;
                    }

                    // Ensure popup labels are non-empty and uniquely identified
                    std::string popupItemLabel = (label.empty() ? "Unknown Type" : label) + "##" + entry.name;

                    if (ImGui::Selectable(popupItemLabel.c_str())) {
                        gbe::SerializedData emptyData;
                        gbe::ISerializable* rawInstance = gbe::TypeRegistry::Instantiate(entry.name, emptyData);

                        if (auto* newComponent = dynamic_cast<ComponentBase*>(rawInstance)) {
                            newComponent->SetOwner(selected);
                            // Ensure collider component is attached
                            if (auto* collider = dynamic_cast<Collider*>(newComponent)) {
                                collider->EnsureAttached();
                            }
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

        // Any interaction with a widget inside this window (drag, checkbox, button,
        // add/remove component, etc.) is coalesced into a single undo entry that
        // spans from activation to release, using HierarchyManager's serialized
        // snapshot as the before/after state.
        const bool isAnyItemActive = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGui::IsAnyItemActive();
        if (isAnyItemActive && !m_WasAnyItemActive) {
            HierarchyManager::GetInstance().BeginUndoableAction();
        }
        if (!isAnyItemActive && m_WasAnyItemActive) {
            HierarchyManager::GetInstance().EndUndoableAction();
        }
        m_WasAnyItemActive = isAnyItemActive;
    }
    ImGui::End();
}