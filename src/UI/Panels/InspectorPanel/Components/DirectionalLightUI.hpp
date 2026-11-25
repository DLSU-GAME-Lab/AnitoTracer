#pragma once

#include "IComponentUI.hpp"
#include "LightUIUtils.hpp"
#include "../../../../Objects/Components/Lights/DirectionLight.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

class DirectionalLightUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        // The registry ensures we only receive DirectionalLight components here.
        DirectionalLight* light = static_cast<DirectionalLight*>(component);

        if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {

            // Edit Light Color
            glm::vec3 color = light->GetColor();
            if (ImGui::ColorEdit3("Color", glm::value_ptr(color))) {
                light->SetColor(color);
            }

            if (LightUI::UsesRealUnits()) {
                float lux = light->GetLux();
                if (ImGui::DragFloat("Lux", &lux, 100.0f, 0.0f, 200000.0f, "%.0f lx")) {
                    light->SetLux(lux);
                }
                LightUI::Tooltip("The light that reaches a surface facing the light. Direct sun is about 100,000 lx, an overcast day 10,000.\n"
                    "How bright it looks depends on the exposure (User Settings, Physical Lights, Exposure).");
            } else {
                LightUI::DrawIntensity(light, LightUI::kIntensityTip);
            }

            float angularRadius = light->GetAngularRadius();
            if (ImGui::DragFloat("Angular Radius", &angularRadius, 0.01f, 0.0f, 10.0f, "%.2f deg")) {
                light->SetAngularRadius(angularRadius);
            }
            LightUI::Tooltip("Size of the light's disk in the sky. The sun is 0.27 degrees. A bigger disk casts softer shadows and a bigger highlight on glossy surfaces.\n"
                "Needs Light Size and Highlight Shape under User Settings, Physical Lights; without them every light has the old fixed size.");

            ImGui::Separator();
            ImGui::TextDisabled("Direction is controlled by the Transform rotation.");
        }
    }
};
