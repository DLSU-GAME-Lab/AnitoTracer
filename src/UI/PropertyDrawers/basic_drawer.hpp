#pragma once

#include <imgui.h>
#include "PropertyDrawer.hpp"
#include <glm/vec3.hpp> // Complex types

namespace gbe {

    // FLOAT
    template <>
    struct PropertyDrawer<float> {
        static bool Draw(const std::string& label, float& target) {
             ImGui::DragFloat(label.c_str(), &target, 0.1f);

             return true;
        }
    };

    // INT
    template <>
    struct PropertyDrawer<int> {
        static bool Draw(const std::string& label, int& target) {
            ImGui::DragInt(label.c_str(), &target);

            return true;
        }
    };

    // BOOL
    template <>
    struct PropertyDrawer<bool> {
        static bool Draw(const std::string& label, bool& target) {
            ImGui::Checkbox(label.c_str(), &target);
            return true;
        }
    };

    // STD::STRING
    template <>
    struct PropertyDrawer<std::string> {
        static bool Draw(const std::string& label, std::string& target) {
            char buffer[256];
            strncpy(buffer, target.c_str(), sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';

            if (ImGui::InputText(label.c_str(), buffer, sizeof(buffer))) {
                target = buffer;
            }
            return true;
        }
    };
}