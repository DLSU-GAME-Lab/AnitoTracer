#pragma once

#include "IComponentUI.hpp"
#include "../../../../Objects/Components/ModelComponent.hpp"
#include "../../../PropertyDrawers/asset_drawer.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

class ModelUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        auto* modelComponent = static_cast<ModelComponent*>(component);
        auto& modelReference = modelComponent->GetModelRef();

        if (!ImGui::CollapsingHeader("Model Component", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        gbe::PropertyDrawer<gbe::AssetRef<Model>>::Draw("Model", modelReference);

        Model* model = modelReference.Get();
        if (!model) {
            ImGui::TextDisabled("No model loaded.");
            return;
        }

        ImGui::Separator();
        DrawMaterialOverride("Roughness", modelComponent,
            &ModelComponent::GetOverrideRoughness, &ModelComponent::SetOverrideRoughness,
            &ModelComponent::GetRoughness, &ModelComponent::SetRoughness);
        DrawMaterialOverride("Metallic", modelComponent,
            &ModelComponent::GetOverrideMetallic, &ModelComponent::SetOverrideMetallic,
            &ModelComponent::GetMetallic, &ModelComponent::SetMetallic);
        DrawEmissiveOverride(modelComponent);

        ImGui::Separator();
        ImGui::Text("Path: %s", model->GetPath().generic_string().c_str());
        ImGui::Text("Submeshes: %zu", model->SubMeshes.size());
        ImGui::Text("PBR Materials: %zu", model->PBRMaterials.size());
        ImGui::Text("PBR Enabled: %s", model->HasPBRProperties ? "True" : "False");

        int vertices = 0;
        for (const auto& submesh : model->SubMeshes)
            vertices += static_cast<int>(submesh.IndexCount);
        ImGui::Text("Total Indices: %i", vertices);

        ImGui::Text("AABB Min: [%.2f, %.2f, %.2f]",
            model->AABBMin.x, model->AABBMin.y, model->AABBMin.z);
        ImGui::Text("AABB Max: [%.2f, %.2f, %.2f]",
            model->AABBMax.x, model->AABBMax.y, model->AABBMax.z);
    }

private:
    // Checkbox to enable the override, plus a 0-1 slider that is greyed out while it's off
    static void DrawMaterialOverride(const char* label, ModelComponent* component,
        bool (ModelComponent::*getEnabled)() const, void (ModelComponent::*setEnabled)(bool),
        float (ModelComponent::*getValue)() const, void (ModelComponent::*setValue)(float))
    {
        ImGui::PushID(label);

        bool enabled = (component->*getEnabled)();
        if (ImGui::Checkbox("##Override", &enabled))
            (component->*setEnabled)(enabled);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Override the material's %s on every submesh.", label);

        ImGui::SameLine();
        ImGui::BeginDisabled(!enabled);
        float value = (component->*getValue)();
        if (ImGui::SliderFloat(label, &value, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            (component->*setValue)(value);
        ImGui::EndDisabled();

        ImGui::PopID();
    }

    // Same layout as DrawMaterialOverride: checkbox, then a color and an unbounded strength greyed out while it's off
    static void DrawEmissiveOverride(ModelComponent* component)
    {
        ImGui::PushID("Emissive");

        bool enabled = component->GetOverrideEmissive();
        if (ImGui::Checkbox("##Override", &enabled))
            component->SetOverrideEmissive(enabled);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Override the material's emissive color and strength on every submesh.\nEmissive maps still mask the glow.");

        ImGui::SameLine();
        ImGui::BeginDisabled(!enabled);
        glm::vec3 color = component->GetEmissiveColor();
        if (ImGui::ColorEdit3("Emissive", glm::value_ptr(color)))
            component->SetEmissiveColor(color);

        float strength = component->GetEmissiveStrength();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::DragFloat("Strength", &strength, 0.05f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            component->SetEmissiveStrength(strength);
        ImGui::EndDisabled();

        ImGui::PopID();
    }
};
