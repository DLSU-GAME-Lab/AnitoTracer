#pragma once

#include <imgui.h>
#include "PropertyDrawer.hpp"
#include "AssetRef.hpp"

#include <algorithm>
#include <typeinfo>
#include <vector>

namespace gbe {

    template <typename T>
    struct PropertyDrawer<gbe::AssetRef<T>> {
        static bool Draw(const std::string& label, gbe::AssetRef<T>& target) {
            bool changed = false;

            ImGui::PushID(label.c_str());

            const GUID currentGuid = target.GetGUID();
            T* currentAsset = target.Get();
            std::string preview = "None (" + std::string(typeid(T).name()) + ")";
            if (currentAsset) {
                preview = currentAsset->GetPath().filename().string();
                if (preview.empty()) preview = currentAsset->GetAssetId();
            }
            else if (!target.IsEmpty()) {
                preview = "Missing Asset (" + currentGuid.ToString() + ")";
            }

            if (ImGui::BeginCombo(label.c_str(), preview.c_str())) {
                if (ImGui::Selectable("None", target.IsEmpty())) {
                    if (!target.IsEmpty()) {
                        target.SetGUID(GUID::Empty());
                        changed = true;
                    }
                }

                ImGui::Separator();

                std::vector<const std::pair<const GUID, IAsset*>*> candidates;
                const auto& assets = AssetDatabase::GetGuidMap();
                for (const auto& assetEntry : assets) {
                    if (assetEntry.second && dynamic_cast<T*>(assetEntry.second))
                        candidates.push_back(&assetEntry);
                }
                std::sort(candidates.begin(), candidates.end(), [](const auto* left, const auto* right) {
                    return left->second->GetPath().generic_string() < right->second->GetPath().generic_string();
                });

                for (const auto* assetEntry : candidates) {
                    const GUID& guid = assetEntry->first;
                    IAsset* asset = assetEntry->second;
                    std::string itemLabel = asset->GetPath().generic_string();
                    if (itemLabel.empty()) itemLabel = asset->GetAssetId();
                    if (itemLabel.empty()) itemLabel = guid.ToString();

                    ImGui::PushID(guid.ToString().c_str());
                    if (ImGui::Selectable(itemLabel.c_str(), currentGuid == guid)) {
                        if (currentGuid != guid) {
                            target.SetGUID(guid);
                            changed = true;
                        }
                    }
                    ImGui::PopID();
                }

                ImGui::EndCombo();
            }

            ImGui::PopID();
            return changed;
        }
    };
}