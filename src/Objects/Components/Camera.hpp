#pragma once

#include "ComponentBase.hpp"
#include "Transform.hpp" // Required for fetching position and rotation!

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED       

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class CameraComponent : public ComponentBase {
public:
    // We require a Transform pointer to ensure the camera always knows where it is!
    CameraComponent(Transform* transform, HierarchyObject* owner = nullptr);

    ~CameraComponent() override = default;

    // Delete copy/assignment to match the base class restrictions.
    CameraComponent(const CameraComponent&) = delete;
    CameraComponent& operator=(const CameraComponent&) = delete;

    CameraComponent(CameraComponent&&) = default;
    CameraComponent& operator=(CameraComponent&&) = default;

    // Projection parameters
    void SetFOV(float fovDegrees) { m_FOV = fovDegrees; }
    void SetAspect(float aspect) { m_Aspect = aspect; }
    void SetNearPlane(float nearZ) { m_NearZ = nearZ; }
    void SetFarPlane(float farZ) { m_FarZ = farZ; }

    float GetFOV() const { return m_FOV; } 
    float GetAspect() const { return m_Aspect; } 
    float GetNearPlane() const { return m_NearZ; } 
    float GetFarPlane() const { return m_FarZ; } 

    // Update matrices
    void UpdateViewMatrix();  
    void UpdateProjectionMatrix();  

    const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; } 
    const glm::mat4& GetProjectionMatrix() const { return m_ProjMatrix; } 

    glm::mat4 GetViewProjectionMatrix() const;

private:
    Transform* m_transform; // The required transform dependency

    float m_FOV = 45.0f;  
    float m_Aspect = 16.0f / 9.0f;  
    float m_NearZ = 0.1f;  
    float m_FarZ = 1000.0f;  

    glm::mat4 m_ViewMatrix = glm::mat4(1.0f);  
    glm::mat4 m_ProjMatrix = glm::mat4(1.0f);  
};