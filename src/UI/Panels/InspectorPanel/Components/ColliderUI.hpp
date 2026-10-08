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
            case IPhysicsEngine::ShapeType::Mesh:
            case IPhysicsEngine::ShapeType::ConvexHull: {
                MeshSource source = collider->GetMeshSource();
                if (gbe::PropertyDrawer<MeshSource>::Draw("Mesh Source", source)) {
                    collider->SetMeshSource(source);
                }

                if (source == MeshSource::FromPath) {
                    std::string path = collider->GetMeshPath();
                    // Adjust this call to whatever string-field drawer the rest of the
                    // codebase uses for text input (plain ImGui::InputText shown here
                    // as a minimal fallback).
                    char buf[260];
                    strncpy_s(buf, path.c_str(), sizeof(buf) - 1);
                    if (ImGui::InputText("Mesh Path", buf, sizeof(buf))) {
                        collider->SetMeshPath(buf);
                    }
                }
                break;
            }
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