#pragma once

#include "IComponentUI.hpp"
#include "../../../../Objects/Components/Camera.hpp" 
#include <imgui.h>

class CameraUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        // The registry safely ensures we only receive CameraComponent instances here.
        CameraComponent* camera = static_cast<CameraComponent*>(component);

        if (ImGui::CollapsingHeader("Camera Component", ImGuiTreeNodeFlags_DefaultOpen)) {

            float fov = camera->GetFOV();
            // DragFloat allows smooth scrubbing. We clamp FOV between 1 and 179 to prevent math singularities!
            if (ImGui::DragFloat("FOV (Degrees)", &fov, 0.5f, 1.0f, 179.0f)) {
                camera->SetFOV(fov);
            }

            float aspect = camera->GetAspect();
            if (ImGui::DragFloat("Aspect Ratio", &aspect, 0.01f, 0.1f, 21.0f)) {
                camera->SetAspect(aspect);
            }

            float nearZ = camera->GetNearPlane();
            if (ImGui::DragFloat("Near Plane", &nearZ, 0.01f, 0.001f, 100.0f)) {
                camera->SetNearPlane(nearZ);
            }

            float farZ = camera->GetFarPlane();
            if (ImGui::DragFloat("Far Plane", &farZ, 1.0f, 1.0f, 10000.0f)) {
                camera->SetFarPlane(farZ);
            }
        }
    }
};