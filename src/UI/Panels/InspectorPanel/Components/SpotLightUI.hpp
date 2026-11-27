#pragma once

#include "IComponentUI.hpp"
#include "LightUIUtils.hpp"
#include "../../../../Objects/Components/Lights/SpotLight.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

class SpotLightUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        // The registry ensures we only receive SpotLight components here.
        SpotLight* light = static_cast<SpotLight*>(component);

        if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {

            glm::vec3 color = light->GetColor();
            if (ImGui::ColorEdit3("Color", glm::value_ptr(color))) {
                light->SetColor(color);
            }

            if (LightUI::UsesRealUnits()) {
                float lumens = light->GetLumens();
                if (ImGui::DragFloat("Lumens", &lumens, 5.0f, 0.0f, 100000.0f, "%.0f lm")) {
                    light->SetLumens(lumens);
                }
                LightUI::Tooltip("The light a lamp of this brightness would give off in all directions: the cone only keeps part of it, "
                    "so a narrower cone does not make the light brighter. A household bulb is about 800 lm.\n"
                    "How bright it looks depends on the exposure (User Settings, Physical Lights, Exposure).");
                ImGui::TextDisabled("%.1f cd", LightUnits::LumensToCandela(lumens));
            } else {
                LightUI::DrawIntensity(light, LightUI::kIntensityTip);
            }

            float range = light->GetRange();
            if (ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 1000.0f, "%.1f m")) {
                light->SetRange(range);
            }
            LightUI::Tooltip("Beyond this distance the light adds nothing. It fades out smoothly toward it.");

            float outer = light->GetOuterAngle();
            if (ImGui::DragFloat("Outer Angle", &outer, 0.2f, 1.0f, 89.0f, "%.1f deg")) {
                light->SetOuterAngle(outer);
                if (light->GetInnerAngle() > outer) light->SetInnerAngle(outer);
            }
            LightUI::Tooltip("Half-angle of the cone: the light is gone at this angle from its axis.");

            float inner = light->GetInnerAngle();
            if (ImGui::DragFloat("Inner Angle", &inner, 0.2f, 0.0f, light->GetOuterAngle(), "%.1f deg")) {
                light->SetInnerAngle(inner);
            }
            LightUI::Tooltip("The light is at full strength inside this angle and fades out smoothly between it and the outer angle.");

            float radius = light->GetRadius();
            if (LightUI::DrawRadius(radius)) {
                light->SetRadius(radius);
            }

            ImGui::Separator();
            ImGui::TextDisabled("Position and direction are controlled by the Transform. It shines along -Y.");
        }
    }
};
