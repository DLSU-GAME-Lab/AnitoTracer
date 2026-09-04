#pragma once

#include <imgui.h>
#include "PropertyDrawer.hpp"

namespace gbe {
	//We shouldnt be able to draw a ComponentBase, but we need to provide a specialization to avoid compiler errors
    template <>
    struct PropertyDrawer<ComponentBase> {
        static bool Draw(const std::string&, ComponentBase&) {
            ImGui::Text("No Renderer for ComponentBase");
            return false;
        }
    };
}