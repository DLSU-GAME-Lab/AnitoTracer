#pragma once

#include <imgui.h>
#include "PropertyDrawer.hpp"
#include <glm/vec3.hpp> // Complex types

namespace gbe {


    // COMPLEX TYPE: GLM::VEC3
    template <>
    struct PropertyDrawer<glm::vec3> {
        static bool Draw(const std::string& label, glm::vec3& target) {
             ImGui::DragFloat3(label.c_str(), &target.x, 0.1f);

             return true;
        }
    };
}