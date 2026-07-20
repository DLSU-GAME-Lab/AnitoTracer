#include "CameraObj.hpp"

void CameraObj::UpdateViewMatrix()
{
    m_ViewMatrix = glm::lookAt(m_Position, m_Target, m_Up);
}

void CameraObj::UpdateProjectionMatrix()
{
    m_ProjMatrix = glm::perspective(glm::radians(m_FOV), m_Aspect, m_NearZ, m_FarZ);
}