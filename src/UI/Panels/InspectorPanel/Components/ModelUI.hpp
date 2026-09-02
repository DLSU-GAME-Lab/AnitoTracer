#pragma once

#include "IComponentUI.hpp"
#include "../../../../Objects/Components/ModelComponent.hpp"
#include "../../../PropertyDrawers/asset_drawer.hpp"
#include <imgui.h>

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
};
