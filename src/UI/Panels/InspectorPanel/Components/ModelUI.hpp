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
        DrawMaterialOverride("Alpha", modelComponent,
            &ModelComponent::GetOverrideAlpha, &ModelComponent::SetOverrideAlpha,
            &ModelComponent::GetAlpha, &ModelComponent::SetAlpha);
        if (modelComponent->GetOverrideAlpha()) {
            // Wrapped: this panel is narrow
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("Drawn as glass. Only the Hybrid and Deferred Hybrid renderers draw it.");
            ImGui::PopStyleColor();
        }

        // Layers over the base material
        DrawMaterialOverride("Clearcoat", modelComponent,
            &ModelComponent::GetOverrideClearcoat, &ModelComponent::SetOverrideClearcoat,
            &ModelComponent::GetClearcoat, &ModelComponent::SetClearcoat);
        DrawIndentedSlider("Coat Roughness", modelComponent->GetOverrideClearcoat(),
            modelComponent->GetClearcoatRoughness(), [&](float value) { modelComponent->SetClearcoatRoughness(value); });
        DrawSheenOverride(modelComponent);
        DrawMaterialOverride("Transmission", modelComponent,
            &ModelComponent::GetOverrideTransmission, &ModelComponent::SetOverrideTransmission,
            &ModelComponent::GetTransmission, &ModelComponent::SetTransmission);
        if (modelComponent->GetOverrideTransmission()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
            ImGui::TextWrapped("Lets the view through, tinted by the base color. Only the Hybrid and Deferred Hybrid renderers show it.");
            ImGui::PopStyleColor();
        }

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

    // A 0-1 slider lined up under the slider of DrawMaterialOverride, greyed out while that override is off
    template <typename Setter>
    static void DrawIndentedSlider(const char* label, bool enabled, float value, Setter setValue)
    {
        ImGui::BeginDisabled(!enabled);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x);
        if (ImGui::SliderFloat(label, &value, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
            setValue(value);
        ImGui::EndDisabled();
    }

    // Checkbox, then the sheen color and how wide its rim is, greyed out while it's off
    static void DrawSheenOverride(ModelComponent* component)
    {
        ImGui::PushID("Sheen");

        bool enabled = component->GetOverrideSheen();
        if (ImGui::Checkbox("##Override", &enabled))
            component->SetOverrideSheen(enabled);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Override the material's sheen (the soft rim of light on cloth) on every submesh.");

        ImGui::SameLine();
        ImGui::BeginDisabled(!enabled);
        glm::vec3 color = component->GetSheenColor();
        if (ImGui::ColorEdit3("Sheen", glm::value_ptr(color)))
            component->SetSheenColor(color);
        ImGui::EndDisabled();

        DrawIndentedSlider("Sheen Roughness", enabled, component->GetSheenRoughness(),
            [&](float value) { component->SetSheenRoughness(value); });

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
