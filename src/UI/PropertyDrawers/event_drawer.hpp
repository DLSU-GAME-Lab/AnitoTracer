#pragma once
#include <imgui.h>

#include "AssignableEvent/MethodRegistry.hpp"
#include "AssignableEvent/AssignableEvent.hpp"

#include "objectref_drawer.hpp"

namespace gbe {

    template <>
    struct PropertyDrawer<UnityEvent> {
        static bool Draw(const std::string& label, UnityEvent& eventTarget) {
            bool changed = false;

            ImGui::PushID(label.c_str());
            ImGui::Text("%s", label.c_str());
            ImGui::Indent();

            // 1. Draw ObjectRef Selector using your existing specialized drawer
            if (PropertyDrawer<gbe::ObjectRef<ISerializable>>::Draw("Target Object", eventTarget.targetObject)) {
                changed = true;
                // Clear the method if the target changes, as the new target might not have it
                eventTarget.targetMethodName.clear();
            }

            // 2. Draw Method Selector
            ISerializable* currentObj = eventTarget.targetObject.Get();
            if (currentObj != nullptr) {

                auto typeIndex = std::type_index(typeid(*currentObj));
                const auto& availableMethods = MethodRegistry::GetInstance().GetAvailableMethods(typeIndex);

                std::string preview = eventTarget.targetMethodName.empty() ? "No Function" : eventTarget.targetMethodName;

                if (ImGui::BeginCombo("Function", preview.c_str())) {

                    if (ImGui::Selectable("No Function", eventTarget.targetMethodName.empty())) {
                        eventTarget.targetMethodName = "";
                        changed = true;
                    }

                    for (const auto& method : availableMethods) {
                        bool isSelected = (eventTarget.targetMethodName == method);
                        if (ImGui::Selectable(method.c_str(), isSelected)) {
                            eventTarget.targetMethodName = method;
                            changed = true;
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            else {
                ImGui::TextDisabled("Select an object to assign a method.");
            }

            ImGui::Unindent();
            ImGui::PopID();
            return changed;
        }
    };
} // namespace gbe