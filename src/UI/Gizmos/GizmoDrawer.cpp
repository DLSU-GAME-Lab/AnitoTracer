#include "GizmoDrawer.hpp"

#include <glm/gtc/type_ptr.hpp>

namespace Diligent {

    void GizmoDrawer::Draw(CameraComponent* pActiveCamera, HierarchyObject::Ref selectedObj, float x, float y, float width, float height)
    {
        if (!selectedObj || !pActiveCamera) return;

        auto* transformComp = selectedObj.GetPtr()->GetComponent<Transform>();
        if (!transformComp) return;

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
        ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());
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
        if (ImGuizmo::IsUsing())
        {
            glm::vec3 pos, rotDegrees, scale;

            ImGuizmo::DecomposeMatrixToComponents(
                glm::value_ptr(objectMatrix),
                glm::value_ptr(pos),
                glm::value_ptr(rotDegrees),
                glm::value_ptr(scale)
            );

            transformComp->SetPosition(pos);
            transformComp->SetEulerAnglesDegrees(rotDegrees);
            transformComp->SetScale(scale);
        }
    }

}