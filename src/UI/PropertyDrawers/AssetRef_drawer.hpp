#pragma once

#include <imgui.h>
#include <string>
#include "PropertyDrawer.hpp"
#include "AssetRef.hpp"
#include "AssetDatabase.hpp"

namespace gbe {

    template <typename T>
    struct PropertyDrawer<AssetRef<T>> {
        static bool Draw(const std::string& label, AssetRef<T>& target) {
            bool modified = false;

            // 1. Get current asset's display name via its GUID / Path
            std::string currentLabel = "None";
            if (target.IsValid()) {
                auto path = target->GetPath();
                currentLabel = path.filename().empty() ? path.string() : path.filename().string();

                // Fallback to GUID string if path is blank
                if (currentLabel.empty()) {
                    currentLabel = target.GetGUID().ToString();
                }
            }

            // 2. Render ImGui Combo Dropdown
            if (ImGui::BeginCombo(label.c_str(), currentLabel.c_str())) {

                // Option: Set to None / Null Reference
                bool isNoneSelected = target.IsEmpty();
                if (ImGui::Selectable("None", isNoneSelected)) {
                    if (!target.IsEmpty()) {
                        target.SetGUID(GUID::Empty());
                        modified = true;
                    }
                }
                if (isNoneSelected) {
                    ImGui::SetItemDefaultFocus();
                }

                // 3. Filter and display assets of type T from AssetDatabase
                for (const auto& [guid, assetPtr] : AssetDatabase::GetGuidMap()) {
                    // Filter assets to only those matching AssetRef's template type T
                    T* typedAsset = dynamic_cast<T*>(assetPtr);
                    if (!typedAsset) {
                        continue;
                    }

                    // Format candidate display name
                    std::string itemLabel;
                    auto path = typedAsset->GetPath();
                    itemLabel = path.filename().empty() ? path.string() : path.filename().string();
                    if (itemLabel.empty()) {
                        itemLabel = guid.ToString();
                    }

                    // Append ##GUID to ensure ImGui label ID uniqueness if files have identical names
                    std::string uniqueItemLabel = itemLabel + "##" + guid.ToString();

                    bool isSelected = (target.GetGUID() == guid);
                    if (ImGui::Selectable(uniqueItemLabel.c_str(), isSelected)) {
                        if (target.GetGUID() != guid) {
                            target.SetGUID(guid);
                            modified = true;
                        }
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }

            return modified;
        }
    };

} // namespace gbe