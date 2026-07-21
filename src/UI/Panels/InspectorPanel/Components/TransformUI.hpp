#pragma once

#include "IComponentUI.hpp"
#include "../../../../Objects/Components/Transform.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

class TransformUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        // The registry ensures we only receive Transform components here.
        Transform* transform = static_cast<Transform*>(component);

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {

            glm::vec3 pos = transform->GetPosition();
            if (ImGui::DragFloat3("Position", glm::value_ptr(pos), 0.1f)) {
                transform->SetPosition(pos);
            }

            glm::vec3 euler = transform->GetEulerAnglesDegrees();
            if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 0.1f)) {
                transform->SetEulerAnglesDegrees(euler);
            }

            glm::vec3 scale = transform->GetScale();
            if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.1f)) {
                transform->SetScale(scale);
            }
        }
    }
};