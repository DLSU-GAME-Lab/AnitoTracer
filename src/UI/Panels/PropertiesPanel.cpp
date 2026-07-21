#include "PropertiesPanel.hpp"

void Diligent::PropertiesPanel::Draw()
{
    ImGui::Begin(m_Name.c_str(), &m_IsVisible);

    if (m_pCamera)
    {
        ImGui::Text("Camera Properties");
        ImGui::Separator();

        // Position
        glm::vec3 position = m_pCamera->GetPosition();
        float posArray[3] = { position.x, position.y, position.z };
        if (ImGui::DragFloat3("Position##camera", posArray, 0.1f))
        {
            m_pCamera->SetPosition(glm::vec3(posArray[0], posArray[1], posArray[2]));
            m_pCamera->UpdateViewMatrix();
        }

        // Target
        glm::vec3 target = m_pCamera->GetTarget();
        float targetArray[3] = { target.x, target.y, target.z };
        if (ImGui::DragFloat3("Target##camera", targetArray, 0.1f))
        {
            m_pCamera->SetTarget(glm::vec3(targetArray[0], targetArray[1], targetArray[2]));
            m_pCamera->UpdateViewMatrix();
        }

        // Up Vector
        glm::vec3 up = m_pCamera->GetUp();
        float upArray[3] = { up.x, up.y, up.z };
        if (ImGui::DragFloat3("Up##camera", upArray, 0.01f))
        {
            m_pCamera->SetUp(glm::vec3(upArray[0], upArray[1], upArray[2]));
            m_pCamera->UpdateViewMatrix();
        }

        ImGui::Separator();
        ImGui::Text("Projection Settings");

        // FOV
        float fov = m_pCamera->GetFOV();
        if (ImGui::SliderFloat("FOV (degrees)", &fov, 1.0f, 180.0f))
        {
            m_pCamera->SetFOV(fov);
            m_pCamera->UpdateProjectionMatrix();
        }

        // Aspect Ratio
        float aspect = m_pCamera->GetAspect();
        if (ImGui::SliderFloat("Aspect Ratio", &aspect, 0.1f, 10.0f))
        {
            m_pCamera->SetAspect(aspect);
            m_pCamera->UpdateProjectionMatrix();
        }

        // Near Plane
        float nearZ = m_pCamera->GetNearPlane();
        if (ImGui::SliderFloat("Near Plane", &nearZ, 0.01f, 10.0f))
        {
            m_pCamera->SetNearPlane(nearZ);
            m_pCamera->UpdateProjectionMatrix();
        }

        // Far Plane
        float farZ = m_pCamera->GetFarPlane();
        if (ImGui::SliderFloat("Far Plane", &farZ, 100.0f, 10000.0f))
        {
            m_pCamera->SetFarPlane(farZ);
            m_pCamera->UpdateProjectionMatrix();
        }
    }
    else
    {
        ImGui::Text("No camera assigned");
    }

    ImGui::End();
}
