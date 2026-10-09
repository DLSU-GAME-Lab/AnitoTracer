#pragma once

#include <imgui.h>
#include <vector>
#include "ImGuizmo.h"

#include "../../Objects/HierarchyManager.hpp"
#include "../../Objects/Components/Camera.hpp"

namespace Diligent {

    class GizmoDrawer {
    public:
        GizmoDrawer() = default;
        ~GizmoDrawer() = default;

        GizmoDrawer(const GizmoDrawer&) = delete;
        GizmoDrawer& operator=(const GizmoDrawer&) = delete;

        // Renders and handles interaction for the gizmo
        void Draw(CameraComponent* pActiveCamera, HierarchyObject::Ref selectedObj, float x, float y, float width, float height);

        // Single gizmo that transforms all roots as a temporary group around their centroid.
        void DrawGroup(CameraComponent* pActiveCamera, const std::vector<HierarchyObject::Ref>& roots,
            HierarchyObject::Ref primary, float x, float y, float width, float height);

        // Getters and Setters for Gizmo state
        ImGuizmo::OPERATION GetOperation() const { return m_CurrentOperation; }
        void SetOperation(ImGuizmo::OPERATION op) { m_CurrentOperation = op; }

        ImGuizmo::MODE GetMode() const { return m_CurrentMode; }
        void SetMode(ImGuizmo::MODE mode) { m_CurrentMode = mode; }

    private:
        // Encapsulated state instead of static variables
        ImGuizmo::OPERATION m_CurrentOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE m_CurrentMode = ImGuizmo::LOCAL;
        bool m_WasUsingGizmo = false;

        struct GroupMember {
            HierarchyObject::Ref object;
            glm::vec3 position;
            glm::quat rotation;
            glm::vec3 localScale;
        };
        // Snapshot taken when a group drag starts; deltas are applied relative to it.
        std::vector<GroupMember> m_GroupMembers;
        glm::mat4 m_GroupPivotStart = glm::mat4(1.0f);
        glm::mat4 m_GroupMatrix = glm::mat4(1.0f);
    };

}