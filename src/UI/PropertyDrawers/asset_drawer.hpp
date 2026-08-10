#pragma once

#include <imgui.h>
#include "PropertyDrawer.hpp"
#include "../../Rendering/Models/ModelManager.hpp"
#include "AssetRef.hpp"

namespace gbe {


    template <>
    struct PropertyDrawer<gbe::AssetRef<Model>> {
        static bool Draw(const std::string& label, gbe::AssetRef<Model>& target) {
            bool changed = false;

            ImGui::PushID(label.c_str());
            ImGui::Separator();

            // Check if the asset reference actually contains a loaded model
            if (target.IsEmpty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Status: No Model Loaded / Failed");
            }
            else {
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Status: Loaded");

                // Safely retrieve the raw model pointer to display its statistics
                Model* modelData = target.Get();
                if (modelData) {
                    ImGui::Indent();

                    // Display internal array sizes
                    ImGui::Text("Path: %s", modelData->GetPath().string());
                    ImGui::Text("Submeshes: %zu", modelData->SubMeshes.size());
                    ImGui::Text("PBR Materials: %zu", modelData->PBRMaterials.size());
                    ImGui::Text("PBR Enabled: %s", modelData->HasPBRProperties ? "True" : "False");

                    ImGui::Spacing();

                    // Display raytracing / culling bounds
                    ImGui::Text("AABB Min: [%.2f, %.2f, %.2f]",
                        modelData->AABBMin.x, modelData->AABBMin.y, modelData->AABBMin.z);
                    ImGui::Text("AABB Max: [%.2f, %.2f, %.2f]",
                        modelData->AABBMax.x, modelData->AABBMax.y, modelData->AABBMax.z);

                    ImGui::Unindent();
                }
            }

            ImGui::Spacing();

            ImGui::PopID();
            return changed;
        }
    };

    //Fallback for other AssetRefs
    template <typename T>
    struct PropertyDrawer<gbe::AssetRef<T>> {
        static bool Draw(const std::string& label, gbe::AssetRef<T>& target) {
            bool changed = false;

            ImGui::PushID(label.c_str());

            ImGui::Separator();

            // Safely check if the asset is loaded using IsEmpty()
            if (target.IsEmpty()) {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Status: Unassigned");
            }
            else {
                ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Status: Loaded");

                ImGui::Indent();
                // Extract generic type info and the raw memory address
                // Will show null or weird values if it fails
                ImGui::TextDisabled("Type: %s", typeid(T).name());
                ImGui::TextDisabled("Ptr: %p", static_cast<void*>(target.Get()));
                ImGui::Unindent();
            }

            ImGui::Spacing();

            ImGui::PopID();
            return changed;
        }
    };
}