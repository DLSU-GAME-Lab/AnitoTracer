#pragma once

#include "IComponentUI.hpp"
#include "MaterialOverrideUI.hpp"
#include "../../../../Objects/Components/ModelComponent.hpp"
#include "../../../../Objects/Components/SubMeshComponent.hpp"
#include <imgui.h>

// One submesh of an object's model, which has a node of its own in the Hierarchy: what it is, and the material properties
// it overrides on its own.
class SubMeshUI : public IComponentUI {
public:
    void Draw(ComponentBase* component) override {
        auto* part = static_cast<SubMeshComponent*>(component);

        if (!ImGui::CollapsingHeader("Submesh Component", ImGuiTreeNodeFlags_DefaultOpen))
            return;

        if (part->IsOrphaned()) {
            MaterialOverrideUI::DrawHint("This node no longer stands for a submesh: the model of its object has changed. "
                "It does nothing now. Delete it, or keep what is attached to it.");
        }

        const SubMesh* submesh = part->GetSubMesh();
        ImGui::Text("Submesh: %d", part->GetSubMeshIndex());
        if (!part->GetSubMeshName().empty())
            ImGui::Text("Name in the model: %s", part->GetSubMeshName().c_str());
        if (submesh) {
            ImGui::Text("Triangles: %u", submesh->IndexCount / 3u);
            ImGui::Text("Material: %u", submesh->MaterialIndex);
        }

        ImGui::Separator();
        const MaterialOverrideUI::Inherited inherited = ComputeInherited(*part);
        MaterialOverrideUI::DrawAll(part, &inherited);

        MaterialOverrideUI::DrawHint("A property left off follows the object's Model component, and the material where that "
            "has no override either.");

        ImGui::BeginDisabled(!part->HasAnyOverride());
        if (ImGui::Button("Reset Overrides"))
            part->ClearOverrides();
        ImGui::EndDisabled();
    }

private:
    // What the submesh has without its own overrides: its material's values, under the object's overrides
    static MaterialOverrideUI::Inherited ComputeInherited(const SubMeshComponent& part) {
        MaterialOverrideUI::Inherited inherited;

        ModelComponent* modelComponent = part.GetModelComponent();
        const Model* model = modelComponent ? modelComponent->GetModel().Get() : nullptr;
        const SubMesh* submesh = part.GetSubMesh();
        if (model && submesh && submesh->MaterialIndex < model->PBRMaterials.size()) {
            const PBRMaterial& material = model->PBRMaterials[submesh->MaterialIndex];
            inherited.Roughness = material.RoughnessFactor;
            inherited.Metallic = material.MetallicFactor;
            inherited.EmissiveColor = glm::vec3(material.EmissiveFactor.r, material.EmissiveFactor.g, material.EmissiveFactor.b);
            inherited.EmissiveStrength = material.EmissiveStrength;
            inherited.Alpha = material.BaseColorFactor.a;
            inherited.Clearcoat = material.Clearcoat;
            inherited.ClearcoatRoughness = material.ClearcoatRoughness;
            inherited.SheenColor = glm::vec3(material.SheenColor.r, material.SheenColor.g, material.SheenColor.b);
            inherited.SheenRoughness = material.SheenRoughness;
            inherited.Transmission = material.Transmission;
        }

        if (modelComponent) {
            if (modelComponent->GetOverrideRoughness()) inherited.Roughness = modelComponent->GetRoughness();
            if (modelComponent->GetOverrideMetallic()) inherited.Metallic = modelComponent->GetMetallic();
            if (modelComponent->GetOverrideEmissive()) {
                inherited.EmissiveColor = modelComponent->GetEmissiveColor();
                inherited.EmissiveStrength = modelComponent->GetEmissiveStrength();
            }
            if (modelComponent->GetOverrideAlpha()) inherited.Alpha = modelComponent->GetAlpha();
            if (modelComponent->GetOverrideClearcoat()) {
                inherited.Clearcoat = modelComponent->GetClearcoat();
                inherited.ClearcoatRoughness = modelComponent->GetClearcoatRoughness();
            }
            if (modelComponent->GetOverrideSheen()) {
                inherited.SheenColor = modelComponent->GetSheenColor();
                inherited.SheenRoughness = modelComponent->GetSheenRoughness();
            }
            if (modelComponent->GetOverrideTransmission()) inherited.Transmission = modelComponent->GetTransmission();
        }
        return inherited;
    }
};
