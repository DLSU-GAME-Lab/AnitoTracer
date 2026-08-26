#pragma once

#include "IComponentUI.hpp"
#include "../../../../Objects/Components/Transform.hpp"
#include "../../../../Objects/Components/ITeleportable.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

class TransformUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        // The registry ensures we only receive Transform components here.
        Transform* transform = static_cast<Transform*>(component);

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            glm::vec3 pos = transform->GetPosition();
            glm::vec3 euler = transform->GetEulerAnglesDegrees();
            glm::vec3 scale = transform->GetScale();

			bool posChanged = ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f);
			bool rotChanged = ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 0.1f);
            bool scaleChanged = ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.1f);

            if (posChanged || rotChanged || scaleChanged) {
                transform->SetPosition(pos);
                transform->SetEulerAnglesDegrees(euler);
                transform->SetScale(scale);

				// Teleport object if it has a component that implements ITeleportable
                if (HierarchyObject* owner = transform->GetOwner().GetPtr()) {
                    if (ITeleportable* teleportable = owner->GetComponent<ITeleportable>()) {
                        teleportable->Teleport(pos, glm::quat(glm::radians(euler)));
                    }
                }
            }
        }
    }
};