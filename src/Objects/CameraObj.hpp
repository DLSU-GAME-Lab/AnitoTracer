#pragma once
#include "Graphics/GraphicsEngine/interface/RenderDevice.h"
#include "Graphics/GraphicsEngine/interface/SwapChain.h"
#include "Common/interface/BasicMath.hpp"


class CameraObj
{
public:
    CameraObj() = default;

    // Position and orientation
    void SetPosition(const Diligent::float3& pos) { m_Position = pos; }
    void SetTarget(const Diligent::float3& target) { m_Target = target; }
    void SetUp(const Diligent::float3& up) { m_Up = up; }

    const Diligent::float3& GetPosition() const { return m_Position; }
    const Diligent::float3& GetTarget() const { return m_Target; }
    const Diligent::float3& GetUp() const { return m_Up; }

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
    void UpdateViewMatrix()
    {
        //m_ViewMatrix = Diligent::LookAtRH(m_Position, m_Target, m_Up);
    }

    void UpdateProjectionMatrix()
    {
        //m_ProjMatrix = Diligent::PerspectiveFovRH(m_FOV, m_Aspect, m_NearZ, m_FarZ);
    }

    const Diligent::float4x4& GetViewMatrix() const { return m_ViewMatrix; }
    const Diligent::float4x4& GetProjectionMatrix() const { return m_ProjMatrix; }

    Diligent::float4x4 GetViewProjectionMatrix() const
    {
        return m_ProjMatrix * m_ViewMatrix;
    }

private:
    Diligent::float3 m_Position = { 0.f, 0.f, -5.f };
    Diligent::float3 m_Target = { 0.f, 0.f, 0.f };
    Diligent::float3 m_Up = { 0.f, 1.f, 0.f };

    float m_FOV = 45.0f;
    float m_Aspect = 16.0f / 9.0f;
    float m_NearZ = 0.1f;
    float m_FarZ = 1000.0f;

    std::string name = "Main Camera";

    Diligent::float4x4 m_ViewMatrix;
    Diligent::float4x4 m_ProjMatrix;
};
