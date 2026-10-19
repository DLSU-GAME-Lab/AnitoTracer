#pragma once

#include <imgui.h>
#include "PropertyDrawer.hpp"
#include "../../../Physics/IPhysicsEngine.hpp"
#include <glm/gtc/type_ptr.hpp>

enum class MeshSource;

namespace gbe {

    template <>
    struct PropertyDrawer<IPhysicsEngine::ShapeType> {
        static bool Draw(const std::string& label, IPhysicsEngine::ShapeType& target) {
            static const char* names[] = { "Box", "Sphere", "Capsule", "Mesh", "Convex Hull" };
            int current = static_cast<int>(target);

            bool changed = ImGui::Combo(label.c_str(), &current, names, IM_ARRAYSIZE(names));
            if (changed) {
                target = static_cast<IPhysicsEngine::ShapeType>(current);
            }
            return changed;
        }
    };

    template <>
    struct PropertyDrawer<IPhysicsEngine::ShapeParams> {
        static bool Draw(const std::string& label, IPhysicsEngine::ShapeParams& target) {
            // v.x/v.y/v.z mean different things depending on ShapeType
            // (box: half-extents, sphere: radius in x, capsule: radius in x, half-height in y)
            return ImGui::DragFloat3(label.c_str(), glm::value_ptr(target.v), 0.1f, 0.0f, FLT_MAX);
        }
    };

    template <>
    struct PropertyDrawer<MeshSource> {
        static bool Draw(const std::string& label, MeshSource& target) {
            static const char* names[] = { "From Path", "From Owner" };
            int current = static_cast<int>(target);

            bool changed = ImGui::Combo(label.c_str(), &current, names, IM_ARRAYSIZE(names));
            if (changed) {
                target = static_cast<MeshSource>(current);
            }
            return changed;
        }
    };
}