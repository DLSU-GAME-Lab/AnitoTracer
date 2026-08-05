#pragma once

#include "IComponentUI.hpp"
#include "../../../../Objects/Components/Lights/PointLight.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

class PointLightUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        // The registry ensures we only receive PointLight components here.
        PointLight* light = static_cast<PointLight*>(component);

        if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {

            // Edit Light Color
            glm::vec3 color = light->GetColor();
            if (ImGui::ColorEdit3("Color", glm::value_ptr(color))) {
                light->SetColor(color);
            }

            // Edit Light Intensity
            float intensity = light->GetIntensity();
            if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f)) {
                light->SetIntensity(intensity);
            }

            // Edit Light Range
            float range = light->GetRange();
            if (ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 1000.0f, "%.1f m")) {
                light->SetRange(range);
            }

            ImGui::Separator();
            ImGui::TextDisabled("Position is controlled by the Transform component.");
        }
    }
};