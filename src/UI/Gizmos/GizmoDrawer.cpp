#include "GizmoDrawer.hpp"
#include "../../Objects/Components/ITeleportable.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace Diligent {

    void GizmoDrawer::Draw(CameraComponent* pActiveCamera, HierarchyObject::Ref selectedObj, float x, float y, float width, float height)
    {
        if (!selectedObj || !pActiveCamera) {
            if (m_WasUsingGizmo) {
                HierarchyManager::GetInstance().EndUndoableAction();
                m_WasUsingGizmo = false;
            }
            return;
        }

        auto* transformComp = selectedObj.GetPtr()->GetComponent<Transform>();
        if (!transformComp) {
            if (m_WasUsingGizmo) {
                HierarchyManager::GetInstance().EndUndoableAction();
                m_WasUsingGizmo = false;
            }
            return;
        }

        // Handle hotkeys (only if UI isn't actively capturing text input)
        //TODO- Change hotkeys later
        /*
        if (!ImGui::IsAnyItemActive())
        {
            if (ImGui::IsKeyPressed(ImGuiKey_W)) m_CurrentOperation = ImGuizmo::TRANSLATE;
            if (ImGui::IsKeyPressed(ImGuiKey_E)) m_CurrentOperation = ImGuizmo::ROTATE;
            if (ImGui::IsKeyPressed(ImGuiKey_R)) m_CurrentOperation = ImGuizmo::SCALE;
            if (ImGui::IsKeyPressed(ImGuiKey_T)) m_CurrentMode = (m_CurrentMode == ImGuizmo::LOCAL) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        }
        */

        // Setup ImGuizmo workspace bounds
        ImGuizmo::SetOrthographic(false);
        //For whole window drawing- change for dockables later
        //ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
        //This is for dockables- keeping above for posterity
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(x, y, width, height);

        // Fetch required matrices
        glm::mat4 viewMatrix = pActiveCamera->GetViewMatrix();
        glm::mat4 projMatrix = pActiveCamera->GetProjectionMatrix();
        glm::mat4 objectMatrix = transformComp->GetLocalMatrix();

        // Draw and interact with the gizmo
        ImGuizmo::Manipulate(
            glm::value_ptr(viewMatrix),
            glm::value_ptr(projMatrix),
            m_CurrentOperation,
            m_CurrentMode,
            glm::value_ptr(objectMatrix)
        );

        // Apply transformations back to component
        const bool isUsingGizmo = ImGuizmo::IsUsing();
        if (isUsingGizmo && !m_WasUsingGizmo) {
            HierarchyManager::GetInstance().BeginUndoableAction();
        }

        if (isUsingGizmo)
        {
            glm::vec3 pos, rotDegrees, scale;

            ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(objectMatrix),
                glm::value_ptr(pos),
                glm::value_ptr(rotDegrees),
                glm::value_ptr(scale)
            );

            glm::quat rot = glm::quat(glm::radians(rotDegrees));

			if (auto* movable = selectedObj.GetPtr()->GetComponent<ITeleportable>()) 
            {
				movable->Teleport(pos, rot);
                transformComp->SetScale(scale);
			}
            else 
            {
                transformComp->SetPosition(pos);
                transformComp->SetEulerAnglesDegrees(rotDegrees);
                transformComp->SetScale(scale);
            }
        }

        if (!isUsingGizmo && m_WasUsingGizmo) {
            HierarchyManager::GetInstance().EndUndoableAction();
        }
        m_WasUsingGizmo = isUsingGizmo;
    }

    void GizmoDrawer::DrawGroup(CameraComponent* pActiveCamera, const std::vector<HierarchyObject::Ref>& roots,
        HierarchyObject::Ref primary, float x, float y, float width, float height)
    {
        if (!pActiveCamera || roots.empty()) return;

        // While idle, re-snapshot every frame so the snapshot is the state right before a drag starts.
        if (!m_WasUsingGizmo)
        {
            m_GroupMembers.clear();
            glm::vec3 centroid(0.0f);
            for (const auto& root : roots)
            {
                HierarchyObject* obj = root.GetPtr();
                Transform* t = obj ? obj->GetTransform() : nullptr;
                if (!t) continue;
                m_GroupMembers.push_back({ root, t->GetPosition(), t->GetRotation(), t->GetLocalScale() });
                centroid += t->GetPosition();
            }
            if (m_GroupMembers.empty()) return;
            centroid /= static_cast<float>(m_GroupMembers.size());

            glm::quat pivotRot(1.0f, 0.0f, 0.0f, 0.0f);
            if (m_CurrentMode == ImGuizmo::LOCAL)
            {
                HierarchyObject* primaryObj = primary.GetPtr();
                Transform* pt = primaryObj ? primaryObj->GetTransform() : nullptr;
                pivotRot = pt ? pt->GetRotation() : m_GroupMembers.front().rotation;
            }

            m_GroupPivotStart = glm::translate(glm::mat4(1.0f), centroid) * glm::mat4_cast(pivotRot);
            m_GroupMatrix = m_GroupPivotStart;
        }

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(x, y, width, height);

        glm::mat4 viewMatrix = pActiveCamera->GetViewMatrix();
        glm::mat4 projMatrix = pActiveCamera->GetProjectionMatrix();

        ImGuizmo::Manipulate(
            glm::value_ptr(viewMatrix),
            glm::value_ptr(projMatrix),
            m_CurrentOperation,
            m_CurrentMode,
            glm::value_ptr(m_GroupMatrix)
        );

        const bool isUsingGizmo = ImGuizmo::IsUsing();
        if (isUsingGizmo && !m_WasUsingGizmo) {
            HierarchyManager::GetInstance().BeginUndoableAction();
        }

        if (isUsingGizmo)
        {
            const glm::mat4 delta = m_GroupMatrix * glm::inverse(m_GroupPivotStart);
            const glm::quat deltaRot = glm::normalize(glm::quat_cast(glm::mat3(delta)));

            glm::vec3 groupPos, groupRotDegrees, groupScale;
            ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(m_GroupMatrix),
                glm::value_ptr(groupPos),
                glm::value_ptr(groupRotDegrees),
                glm::value_ptr(groupScale)
            );

            for (const auto& member : m_GroupMembers)
            {
                HierarchyObject* obj = member.object.GetPtr();
                Transform* t = obj ? obj->GetTransform() : nullptr;
                if (!t) continue;

                const glm::vec3 newPos = glm::vec3(delta * glm::vec4(member.position, 1.0f));
                glm::quat newRot = member.rotation;
                glm::vec3 newScale = member.localScale;

                if (m_CurrentOperation == ImGuizmo::ROTATE) {
                    newRot = glm::normalize(deltaRot * member.rotation);
                }
                else if (m_CurrentOperation == ImGuizmo::SCALE) {
                    newScale = member.localScale * groupScale;
                }

                if (auto* movable = obj->GetComponent<ITeleportable>()) {
                    movable->Teleport(newPos, newRot);
                }
                else {
                    t->SetWorldPosition(newPos);
                    t->SetWorldRotation(newRot);
                }
                t->SetScale(newScale);
            }
        }

        if (!isUsingGizmo && m_WasUsingGizmo) {
            HierarchyManager::GetInstance().EndUndoableAction();
        }
        m_WasUsingGizmo = isUsingGizmo;
    }

}