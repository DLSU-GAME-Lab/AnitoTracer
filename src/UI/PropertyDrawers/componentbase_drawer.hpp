#pragma once

#include <imgui.h>
#include "PropertyDrawer.hpp"
#include <unordered_map>

namespace gbe {
	//We shouldnt be able to draw a ComponentBase, but we need to provide a specialization to avoid compiler errors
    template <>
    struct PropertyDrawer<ComponentBase> {
        static bool Draw(const std::string&, ComponentBase&) {
            ImGui::Text("No Renderer for ComponentBase");
            return false;
        }
    };

    template <>
    struct PropertyDrawer<std::unordered_map<std::string, std::string>> {
        static bool Draw(const std::string& label, std::unordered_map<std::string, std::string>&) {
            ImGui::TextDisabled("%s", label.c_str());
            return false;
        }
    };
}