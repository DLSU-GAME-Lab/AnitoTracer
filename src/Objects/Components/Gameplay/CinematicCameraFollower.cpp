#include "CinematicCameraFollower.hpp"

#include "HierarchyObject.hpp"

#include <glm/gtc/quaternion.hpp>
#include <cmath>

CinematicCameraFollower::CinematicCameraFollower(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("CinematicCameraFollower", owner) {}

void CinematicCameraFollower::OnUpdate(float deltaTime) {
    Transform* target = m_target.Get();
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!target || !owner) {
        return;
    }

    Transform* camera = owner->GetTransform();
    if (!camera) {
        return;
    }

    const glm::vec3 targetPos = target->GetPosition();
    const glm::quat targetRot = target->GetRotation();

    const glm::vec3 desiredPos = targetPos + targetRot * m_targetOffset;

    glm::quat desiredRot;
    if (m_matchTargetRotation) {
        desiredRot = targetRot;
    } else {
        const glm::vec3 lookPoint = targetPos + targetRot * m_lookAtOffset;
        const glm::vec3 toLook = lookPoint - desiredPos;
        const float lenSq = glm::dot(toLook, toLook);
        if (lenSq < 1e-8f) {
            desiredRot = camera->GetRotation();
        } else {
            const glm::vec3 forward = toLook / std::sqrt(lenSq);
            // Blend between world-up (level horizon) and target-up (full roll).
            glm::vec3 up = glm::mix(glm::vec3(0.0f, 1.0f, 0.0f), targetRot * glm::vec3(0.0f, 1.0f, 0.0f),
                                    glm::clamp(m_rollInfluence, 0.0f, 1.0f));
            if (std::abs(glm::dot(glm::normalize(up), forward)) > 0.999f) {
                up = targetRot * glm::vec3(0.0f, 1.0f, 0.0f);
            }
            desiredRot = glm::quatLookAt(forward, glm::normalize(up));
        }
    }

    if (!m_initialized) {
        m_initialized = true;
        if (m_snapOnStart) {
            camera->SetWorldPosition(desiredPos);
            camera->SetWorldRotation(desiredRot);
            return;
        }
    }

    // Frame-rate independent exponential smoothing.
    const auto blend = [deltaTime](float smoothness) {
        return smoothness <= 0.0f ? 1.0f : 1.0f - std::exp(-smoothness * deltaTime);
    };

    glm::vec3 newPos = glm::mix(camera->GetPosition(), desiredPos, blend(m_positionSmoothness));
    if (m_maxLagDistance > 0.0f) {
        const glm::vec3 lag = newPos - desiredPos;
        const float lagLen = glm::length(lag);
        if (lagLen > m_maxLagDistance) {
            newPos = desiredPos + lag * (m_maxLagDistance / lagLen);
        }
    }

    const glm::quat newRot = glm::normalize(glm::slerp(camera->GetRotation(), desiredRot, blend(m_rotationSmoothness)));

    camera->SetWorldPosition(newPos);
    camera->SetWorldRotation(newRot);
}
