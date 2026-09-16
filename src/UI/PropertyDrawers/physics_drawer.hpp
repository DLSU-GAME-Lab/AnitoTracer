#pragma once

#include <imgui.h>
#include "PropertyDrawer.hpp"
#include "../../Physics/IPhysicsEngine.hpp"
#include "glm_drawer.hpp"

namespace gbe {

    // IPhysicsEngine::ShapeType (enum): combo box over the known shape kinds.
    template <>
    struct PropertyDrawer<IPhysicsEngine::ShapeType> {
        static bool Draw(const std::string& label, IPhysicsEngine::ShapeType& target) {
            static const char* names[] = { "Box", "Sphere", "Capsule" };
            int index = static_cast<int>(target);
            bool changed = ImGui::Combo(label.c_str(), &index, names, IM_ARRAYSIZE(names));
            if (changed) {
                target = static_cast<IPhysicsEngine::ShapeType>(index);
            }
            return changed;
        }
    };

    // IPhysicsEngine::ShapeParams: single vec3 whose meaning depends on ShapeType
    // (box: half extents, sphere: radius in x, capsule: radius in x / half-height in y).
    template <>
    struct PropertyDrawer<IPhysicsEngine::ShapeParams> {
        static bool Draw(const std::string& label, IPhysicsEngine::ShapeParams& target) {
            return PropertyDrawer<glm::vec3>::Draw(label, target.v);
        }
    };

}
