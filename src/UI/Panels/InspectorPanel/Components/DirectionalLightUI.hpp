#pragma once

#include "IComponentUI.hpp"
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

            // Edit Light Intensity
            float intensity = light->GetIntensity();
            if (ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100.0f)) {
                light->SetIntensity(intensity);
            }

            ImGui::Separator();
            ImGui::TextDisabled("Direction is controlled by the Transform rotation.");
        }
    }
};