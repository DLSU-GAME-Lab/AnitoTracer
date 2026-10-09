#pragma once

#include <imgui.h>
#include "PropertyDrawer.hpp"

// Included by PrefabRef.hpp after the class definition; do not include directly.

namespace gbe {

    template <>
    struct PropertyDrawer<PrefabRef> {
        static bool Draw(const std::string& label, PrefabRef& target) {
            bool changed = false;

            ImGui::PushID(label.c_str());

            std::string preview = "None (Prefab)";
            if (!target.IsEmpty()) {
                preview = std::filesystem::path(target.GetPath()).filename().string();
                std::error_code error;
                if (!std::filesystem::exists(target.Resolve(), error)) {
                    preview = "Missing Prefab (" + target.GetPath() + ")";
                }
            }

            if (ImGui::BeginCombo(label.c_str(), preview.c_str())) {
                if (ImGui::Selectable("None", target.IsEmpty())) {
                    if (!target.IsEmpty()) {
                        target.SetPath({});
                        changed = true;
                    }
                }

                ImGui::Separator();

                for (const auto& prefabPath : PrefabRef::EnumeratePrefabs()) {
                    const std::string stored = PrefabRef::MakeStorable(prefabPath);

                    ImGui::PushID(stored.c_str());
                    if (ImGui::Selectable(stored.c_str(), target.GetPath() == stored)) {
                        if (target.GetPath() != stored) {
                            target.SetPath(stored);
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
