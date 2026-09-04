#pragma once

#include <imgui.h>
#include "PropertyDrawer.hpp"

#include <memory>
#include <vector>

namespace gbe {
    //We shouldnt be able to draw a vectors, but we need to provide a specialization to avoid compiler errors
    template <typename T>
    struct PropertyDrawer<std::vector<std::unique_ptr<T>>> {
        static bool Draw(const std::string&, std::vector<std::unique_ptr<T>>&) {
            ImGui::Text("No Renderer for vectors");
            return false;
        }
    };
}