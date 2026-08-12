#pragma once

#include ANITO_SERIALIZATION_INCLUDES

#include <imgui.h>
#include "PropertyDrawer.hpp"
#include "ObjectRef.hpp"
#include "SceneRegistry.hpp"
#include <typeinfo>
#include <string>

namespace gbe {

    template <typename T>
    struct PropertyDrawer<gbe::ObjectRef<T>> {
        static bool Draw(const std::string& label, gbe::ObjectRef<T>& target) {
            bool changed = false;

            ImGui::PushID(label.c_str());

            // Get current selection info
            const GUID currentGuid = target.GetTargetGUID();

            T* currentObj = target.Get(); // Lazy-resolves pointer via SceneRegistry
            

            // Construct preview string for the combo header
            std::string previewText;
            if (currentObj != nullptr) {
                std::string currentLabel = currentObj->GetLabel();
                previewText = currentLabel.size() > 0 ? currentLabel : "[" + currentGuid.ToString() + "]";
            }
            else if (currentGuid != GUID::Empty()) {
                previewText = "Missing Reference (" + currentGuid.ToString() + ")";
            }
            else {
                previewText = "None (" + std::string(typeid(T).name()) + ")";
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

                    // Safely check if this memory instance can cast to type T
                    T* typedPtr = dynamic_cast<T*>(rawPtr);
                    if (typedPtr != nullptr) {
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
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::PopID();
            return true;
        }
    };

} // namespace gbe