#pragma once

#include "Components/ComponentBase.hpp"
#include "Components/Transform.hpp"
#include "InputSystem.hpp"
#include <glm/glm.hpp>
#include <algorithm>


#include "UI/CursorManager.hpp"
#include "../AppConfig.hpp"

class PlayerLookComponent : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    PlayerLookComponent(gbe::IInstanceManager<HierarchyObject>::Ref owner = nullptr)
        : ComponentBase("PlayerLookComponent", owner),
        m_sensitivity(0.1f),
        m_pitch(0.0f),
        m_yaw(0.0f) {}

    ~PlayerLookComponent() override = default;

    PlayerLookComponent(const PlayerLookComponent&) = delete;
    PlayerLookComponent& operator=(const PlayerLookComponent&) = delete;

    PlayerLookComponent(PlayerLookComponent&&) = default;
    PlayerLookComponent& operator=(PlayerLookComponent&&) = default;

    void OnUpdate(float) override {
        Transform* transform = m_targetTransform.Get();
        if (!transform) {
            return;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            CursorManager::GetInstance().TemporarilyUnlock();
        }

        gbe::InputSystem::Vec2 mouseDelta = gbe::InputSystem::GetMouseDelta();

        if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) {
            // Horizontal movement rotates around Y-axis (Yaw)
            m_yaw += mouseDelta.x * m_sensitivity;

            // Vertical movement rotates around X-axis (Pitch)
            m_pitch -= mouseDelta.y * m_sensitivity * (m_reverse_y ? -1.0f : 1.0f);

            // Clamp pitch to prevent camera flipping at poles
            m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

            // Apply orientation to Transform
            transform->SetEulerAnglesDegrees(glm::vec3(m_pitch, m_yaw, 0.0f));
        }

        if (m_lockcursor && CursorManager::GetInstance().IsLocked() &&
            !CursorManager::GetInstance().IsTemporarilyUnlocked()) {
            CursorManager::GetInstance().MaintainLock();
        }
    }

    void OnStart() override {
        if (AppConfig::release)
            CursorManager::GetInstance().SetCursorLock(m_lockcursor);
    }

    // Target Transform Accessors
    void SetTargetTransform(gbe::ObjectRef<Transform> target) { m_targetTransform = target; }
    gbe::ObjectRef<Transform> GetTargetTransform() const { return m_targetTransform; }

    // Sensitivity & Angle Configuration
    void SetSensitivity(float sensitivity) { m_sensitivity = sensitivity; }
    float GetSensitivity() const { return m_sensitivity; }

    float GetPitch() const { return m_pitch; }
    void SetPitch(float pitch) { m_pitch = pitch; }

    float GetYaw() const { return m_yaw; }
    void SetYaw(float yaw) { m_yaw = yaw; }

private:
    gbe::ObjectRef<Transform> m_targetTransform = nullptr;
    GBE_SERIALIZE_FIELD_W_NAME(m_targetTransform, "Target Transform");

    float m_sensitivity = 0.1f;
    GBE_SERIALIZE_FIELD_W_NAME(m_sensitivity, "Sensitivity");

    float m_pitch = 0.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_pitch, "Pitch");

    float m_yaw = 0.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_yaw, "Yaw");

    bool m_reverse_y = true;
    GBE_SERIALIZE_FIELD_W_NAME(m_reverse_y, "Reverse Y");
    
    bool m_lockcursor = true;
    GBE_SERIALIZE_FIELD_W_NAME(m_lockcursor, "Lock Cursor");

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(PlayerLookComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(PlayerLookComponent, ComponentBase);