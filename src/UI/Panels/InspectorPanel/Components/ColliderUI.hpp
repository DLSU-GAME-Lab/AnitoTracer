#pragma once

#include <imgui.h>
#include "IComponentUI.hpp"
#include "../../../Objects/Components/Physics/Collider.hpp"

class ColliderUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        Collider* collider = dynamic_cast<Collider*>(component);
        if (!collider) return;

        if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
            IPhysicsEngine::ShapeType type = collider->GetShapeType();
            if (gbe::PropertyDrawer<IPhysicsEngine::ShapeType>::Draw("Shape Type", type)) {
                collider->SetShapeType(type);
            }

            IPhysicsEngine::ShapeParams params = collider->GetShapeParams();
            bool paramsChanged = false;

            switch (type) {
            case IPhysicsEngine::ShapeType::Box:
                paramsChanged = ImGui::DragFloat3("Half Extents", &params.v.x, 0.1f, 0.0f, FLT_MAX);
                break;
            case IPhysicsEngine::ShapeType::Sphere:
                paramsChanged = ImGui::DragFloat("Radius", &params.v.x, 0.1f, 0.0f, FLT_MAX);
                break;
            case IPhysicsEngine::ShapeType::Capsule:
                paramsChanged = ImGui::DragFloat("Radius", &params.v.x, 0.1f, 0.0f, FLT_MAX);
                paramsChanged |= ImGui::DragFloat("Half Height", &params.v.y, 0.1f, 0.0f, FLT_MAX);
                break;
            }

            if (paramsChanged) {
                collider->SetShapeParams(params);
            }

            glm::vec3 offset = collider->GetOffset();
            if (gbe::PropertyDrawer<glm::vec3>::Draw("Offset", offset)) {
                collider->SetOffset(offset);
            }
        }
    }
};