#pragma once

#include ANITO_SERIALIZATION_INCLUDES

#include "ISerializable.hpp"

#include <imgui.h>
#include "PropertyDrawer.hpp"
#include "ObjectRef.hpp"
#include "SceneRegistry.hpp"
#include <string>

namespace gbe {

    template <typename T>
    struct PropertyDrawer<gbe::ObjectRef<T>> {
        static bool Draw(const std::string& label, gbe::ObjectRef<T>& target) {
            bool changed = false;

            ImGui::PushID(label.c_str());

            // Get current selection info
            const GUID currentGuid = target.GetTargetGUID();

            // Construct preview string for the combo header
            std::string previewText;

            if (currentGuid != GUID::Empty()) {
                const auto& registry = SceneRegistry::GetInstance().GetRegistry();
                auto it = registry.find(currentGuid);
                if (it != registry.end() && it->second != nullptr) {
                    const std::string currentLabel = it->second->GetLabel();
                    previewText = currentLabel.size() > 0 ? currentLabel : "[" + currentGuid.ToString() + "]";
                }
                else {
                    previewText = "Missing Reference (" + currentGuid.ToString() + ")";
                }
            } else {
                previewText = "None";
            }

            if (previewText.empty()) {
                previewText = "Missing Reference (" + currentGuid.ToString() + ")";
            }

            // Draw ImGui Dropdown
            if (ImGui::BeginCombo(label.c_str(), previewText.c_str())) {

                // Option 1: Unset / None
                bool isNoneSelected = (currentGuid == GUID::Empty());
                if (ImGui::Selectable("None", isNoneSelected)) {
                    if (currentGuid != GUID::Empty()) {
                        target.SetGUID(GUID::Empty());
                        changed = true;
                    }
                }
                if (isNoneSelected) {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::Separator();

                // Option 2: Iterate memory via SceneRegistry for all valid ISerializables of type T
                const auto& registry = SceneRegistry::GetInstance().GetRegistry();
                for (const auto& [guid, rawPtr] : registry) {
                    if (!rawPtr) continue;

                    const std::string guidId = guid.ToString();
                    ImGui::PushID(guidId.c_str());

                    bool isSelected = (currentGuid == guid);

                    std::string currentItemLabel = rawPtr->GetLabel();
                    std::string itemLabel = currentItemLabel.size() > 0 ? currentItemLabel : "[" + guid.ToString() + "]";

                    if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                        if (currentGuid != guid) {
                            target.SetGUID(guid);
                            changed = true;
                        }
                    }

                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                    ImGui::PopID();
                }

                ImGui::EndCombo();
            }

            ImGui::PopID();
            return changed;
        }
    };

} // namespace gbe