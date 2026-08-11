#pragma once

#include "ComponentBase.hpp"
#include "Transform.hpp" // Required for fetching position and rotation!

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED       

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Organization/IInstanceManager.hpp"

class CameraComponent : public ComponentBase, public gbe::IInstanceManager<CameraComponent> {
public:
    // We require a Transform pointer to ensure the camera always knows where it is!
    CameraComponent(Transform* transform, gbe::IInstanceManager<HierarchyObject>::Ref owner = {});

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
    Transform* m_transform = nullptr; // The required transform dependency

    float m_FOV = 45.0f;  
    GBE_SERIALIZE_FIELD_W_NAME(m_FOV, "FOV");
    float m_Aspect = 16.0f / 9.0f;  
    GBE_SERIALIZE_FIELD_W_NAME(m_Aspect, "Aspect Ratio");
    float m_NearZ = 0.1f;
    GBE_SERIALIZE_FIELD_W_NAME(m_NearZ, "Near Plane");
    float m_FarZ = 1000.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_FarZ, "Far Plane");

    gbe::ObjectRef<HierarchyObject> m_lookAtObj = nullptr;
    gbe::ObjectRef<Transform> m_lookAtTransform = nullptr;
    //TODO: This breaks the build due to missing serializer definition
    // Pls fix
    GBE_SERIALIZE_FIELD_W_NAME(m_lookAtObj, "Look at Object");
    GBE_SERIALIZE_FIELD_W_NAME(m_lookAtTransform, "Look at Transform");
    

    glm::mat4 m_ViewMatrix = glm::mat4(1.0f);  
    glm::mat4 m_ProjMatrix = glm::mat4(1.0f);  

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(CameraComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(CameraComponent, ComponentBase);