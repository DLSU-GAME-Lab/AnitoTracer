#pragma once

#include "IComponentUI.hpp"
#include "LightUIUtils.hpp"
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

            if (LightUI::UsesRealUnits()) {
                float lumens = light->GetLumens();
                if (ImGui::DragFloat("Lumens", &lumens, 5.0f, 0.0f, 100000.0f, "%.0f lm")) {
                    light->SetLumens(lumens);
                }
                LightUI::Tooltip("All the light the lamp gives off. A household bulb is about 800 lm.\n"
                    "How bright it looks depends on the exposure (User Settings, Physical Lights, Exposure).");
                ImGui::TextDisabled("%.1f cd, %.1f lux at 2 m", LightUnits::LumensToCandela(lumens),
                    LightUnits::CandelaToLux(LightUnits::LumensToCandela(lumens), 2.0f));
            } else {
                LightUI::DrawIntensity(light, LightUI::kIntensityTip);
            }

            // Edit Light Range
            float range = light->GetRange();
            if (ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 1000.0f, "%.1f m")) {
                light->SetRange(range);
            }
            LightUI::Tooltip("Beyond this distance the light adds nothing. It fades out smoothly toward it.");

            float radius = light->GetRadius();
            if (LightUI::DrawRadius(radius)) {
                light->SetRadius(radius);
            }

            ImGui::Separator();
            ImGui::TextDisabled("Position is controlled by the Transform component.");
        }
    }
};
