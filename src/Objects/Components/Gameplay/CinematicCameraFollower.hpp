#pragma once

#include "Components/ComponentBase.hpp"
#include "Components/Transform.hpp"
#include "Types/UpdateTrigger.hpp"

#include <glm/glm.hpp>

// Smoothly follows a target transform with a positional offset and rotation matching; attach to the camera object.
class CinematicCameraFollower : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    CinematicCameraFollower(gbe::IInstanceManager<HierarchyObject>::Ref owner = {});
    ~CinematicCameraFollower() override = default;

    CinematicCameraFollower(const CinematicCameraFollower&) = delete;
    CinematicCameraFollower& operator=(const CinematicCameraFollower&) = delete;

    CinematicCameraFollower(CinematicCameraFollower&&) = default;
    CinematicCameraFollower& operator=(CinematicCameraFollower&&) = default;

    void OnUpdate(float deltaTime) override;

private:
    gbe::ObjectRef<Transform> m_target = nullptr;
    GBE_SERIALIZE_FIELD_W_NAME(m_target, "Target");

    // Offset in the target's local space, so the camera stays behind the target as it turns.
    glm::vec3 m_targetOffset = glm::vec3(0.0f, 2.0f, -8.0f);
    GBE_SERIALIZE_FIELD_W_NAME(m_targetOffset, "Target Offset");

    // Offset in the target's local space that the camera looks at.
    glm::vec3 m_lookAtOffset = glm::vec3(0.0f, 0.0f, 10.0f);
    GBE_SERIALIZE_FIELD_W_NAME(m_lookAtOffset, "Look At Offset");

    bool m_matchTargetRotation = false;
    GBE_SERIALIZE_FIELD_W_NAME(m_matchTargetRotation, "Match Target Rotation");

    // Higher is snappier; 0 disables smoothing (instant snap).
    float m_positionSmoothness = 6.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_positionSmoothness, "Position Smoothness");

    float m_rotationSmoothness = 4.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_rotationSmoothness, "Rotation Smoothness");

    // Fraction of the target's roll copied to the camera (0 = level horizon, 1 = full roll).
    float m_rollInfluence = 0.25f;
    GBE_SERIALIZE_FIELD_W_NAME(m_rollInfluence, "Roll Influence");

    // Max distance the camera may lag behind its ideal position; 0 = unlimited.
    float m_maxLagDistance = 0.0f;
    GBE_SERIALIZE_FIELD_W_NAME(m_maxLagDistance, "Max Lag Distance");

    bool m_snapOnStart = true;
    GBE_SERIALIZE_FIELD_W_NAME(m_snapOnStart, "Snap On Start");

    bool m_initialized = false;

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(CinematicCameraFollower, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(CinematicCameraFollower, ComponentBase);
