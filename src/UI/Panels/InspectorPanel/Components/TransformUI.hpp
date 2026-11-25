#pragma once

#include "IComponentUI.hpp"
#include "../../../../Objects/Components/Transform.hpp"
#include "../../../../Objects/Components/ITeleportable.hpp"
#include "../../../../Objects/Components/SubMeshComponent.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

class TransformUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        // The registry ensures we only receive Transform components here.
        Transform* transform = static_cast<Transform*>(component);

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            glm::vec3 pos = transform->GetLocalPosition();
            glm::vec3 euler = transform->GetEulerAnglesDegrees();
            glm::vec3 scale = transform->GetLocalScale();

            // The node of a submesh stands at the middle of its part, and keeps there (HierarchyManager::SyncSubMeshNodes)
            const HierarchyObject* owner = transform->GetOwner().GetPtr();
            const SubMeshComponent* part = owner ? owner->GetComponent<SubMeshComponent>() : nullptr;
            const bool lockedToSubMesh = part && !part->IsOrphaned();

            ImGui::BeginDisabled(lockedToSubMesh);
			bool posChanged = ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f);
			bool rotChanged = ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 0.1f);
            bool scaleChanged = ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.1f);
            ImGui::EndDisabled();

            if (lockedToSubMesh) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::TextWrapped("At the middle of the submesh. What is attached here stays with the part; the part itself "
                    "does not move with this.");
                ImGui::PopStyleColor();
                return;
            }

            if (posChanged || rotChanged || scaleChanged) {
                transform->SetPosition(pos);
                transform->SetEulerAnglesDegrees(euler);
                transform->SetScale(scale);

				// Teleport object if it has a component that implements ITeleportable
                if (HierarchyObject* owner = transform->GetOwner().GetPtr()) {
                    if (ITeleportable* teleportable = owner->GetComponent<ITeleportable>()) {
                        teleportable->Teleport(transform->GetPosition(), transform->GetRotation());
                    }
                    else {
                        transform->SyncChildrenPhysics();
                    }
                }
            }
        }
    }
};