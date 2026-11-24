#pragma once

#include "IComponentUI.hpp"
#include "LightUIUtils.hpp"
#include "../../../../Objects/Components/Lights/AreaLight.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

class AreaLightUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        // The registry ensures we only receive AreaLight components here.
        AreaLight* light = static_cast<AreaLight*>(component);

        if (ImGui::CollapsingHeader("Area Light", ImGuiTreeNodeFlags_DefaultOpen)) {

            glm::vec3 color = light->GetColor();
            if (ImGui::ColorEdit3("Color", glm::value_ptr(color))) {
                light->SetColor(color);
            }

            if (LightUI::UsesRealUnits()) {
                float lumens = light->GetLumens();
                if (ImGui::DragFloat("Lumens", &lumens, 5.0f, 0.0f, 100000.0f, "%.0f lm")) {
                    light->SetLumens(lumens);
                }
                LightUI::Tooltip("All the light the panel gives off, through its front. A bigger panel with the same lumens is dimmer per square meter.\n"
                    "How bright it looks depends on the exposure (User Settings, Physical Lights, Exposure).");
                const float area = light->GetWidth() * light->GetHeight();
                ImGui::TextDisabled("%.0f cd/m2", LightUnits::LumensToRadiance(lumens, area));
            } else {
                LightUI::DrawIntensity(light, "How bright the panel's surface is, in scene units. A bigger panel lights more.\n"
                    "Real Units in User Settings, Physical Lights, sets lights in lumens and lux instead.");
            }

            float width = light->GetWidth();
            if (ImGui::DragFloat("Width", &width, 0.02f, 0.01f, 100.0f, "%.2f m")) {
                light->SetWidth(width);
            }
            float height = light->GetHeight();
            if (ImGui::DragFloat("Height", &height, 0.02f, 0.01f, 100.0f, "%.2f m")) {
                light->SetHeight(height);
            }
            LightUI::Tooltip("The panel lies in the object's X and Z plane and shines along -Y, on its front side only. Scaling the object does not resize it.");

            float range = light->GetRange();
            if (ImGui::DragFloat("Range", &range, 0.1f, 0.0f, 1000.0f, "%.1f m")) {
                light->SetRange(range);
            }
            LightUI::Tooltip("Beyond this distance the light adds nothing. It fades out smoothly toward it.");

            ImGui::Separator();
            ImGui::TextDisabled("Only the Hybrid and Deferred Hybrid renderers shade it as a panel;\n"
                "the others treat it as a point light at its center.");
        }
    }
};
