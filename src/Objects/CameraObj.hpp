#pragma once
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Common/interface/BasicMath.hpp"


#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Matches Vulkan/D3D depth range [0, 1]
#define GLM_FORCE_LEFT_HANDED       // Matches Diligent's default coordinate system


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>


class CameraObj
{
public:
    CameraObj() = default;

    // Position and orientation
    void SetPosition(const glm::vec3& pos) { m_Position = pos; }
    void SetTarget(const glm::vec3& target) { m_Target = target; }
    void SetUp(const glm::vec3& up) { m_Up = up; }

    const glm::vec3& GetPosition() const { return m_Position; }
    const glm::vec3& GetTarget() const { return m_Target; }
    const glm::vec3& GetUp() const { return m_Up; }

    // Projection parameters
    void SetFOV(float fovDegrees) { m_FOV = fovDegrees; }
    void SetAspect(float aspect) { m_Aspect = aspect; }
    void SetNearPlane(float nearZ) { m_NearZ = nearZ; }
    void SetFarPlane(float farZ) { m_FarZ = farZ; }

    float GetFOV() const { return m_FOV; }
    float GetAspect() const { return m_Aspect; }
    float GetNearPlane() const { return m_NearZ; }
    float GetFarPlane() const { return m_FarZ; }
    std::string GetName() const { return name; }

    // Update matrices
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
    const glm::mat4& GetProjectionMatrix() const { return m_ProjMatrix; }

    glm::mat4 GetViewProjectionMatrix() const
    {
        return m_ProjMatrix * m_ViewMatrix;
    }

private:
    glm::vec3 m_Position = glm::vec3(0.f, 0.f, -5.f);
    glm::vec3 m_Target = glm::vec3(0.f, 0.f, 0.f);
    glm::vec3 m_Up = glm::vec3(0.f, 1.f, 0.f);

    float m_FOV = 45.0f;
    float m_Aspect = 16.0f / 9.0f;
    float m_NearZ = 0.1f;
    float m_FarZ = 1000.0f;

    std::string name = "Main Camera";

    glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
    glm::mat4 m_ProjMatrix = glm::mat4(1.0f);
};
