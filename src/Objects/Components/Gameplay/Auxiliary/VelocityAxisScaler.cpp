#include "VelocityAxisScaler.hpp"

#include "HierarchyObject.hpp"

#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cmath>

VelocityAxisScaler::VelocityAxisScaler(gbe::IInstanceManager<HierarchyObject>::Ref owner)
    : ComponentBase("VelocityAxisScaler", owner) {}

void VelocityAxisScaler::OnUpdate(float deltaTime) {
    HierarchyObject* owner = GetOwner().GetPtr();
    Transform* target = m_target.Get();
    Transform* tracked = owner ? owner->GetTransform() : nullptr;
    if (!target || !tracked || deltaTime <= 0.0f) {
        return;
    }

    const glm::vec3 position = tracked->GetPosition();
    if (!m_hasLastPosition) {
        m_lastPosition = position;
        m_hasLastPosition = true;
        return;
    }

    const glm::vec3 velocity = (position - m_lastPosition) / deltaTime;
    m_lastPosition = position;

    // Velocity expressed in the owner's local space.
    const glm::vec3 localVelocity = glm::inverse(tracked->GetRotation()) * velocity;

    glm::vec3 scale = target->GetLocalScale();
    const auto apply = [&](bool enabled, float speed, float& component) {
        if (!enabled) {
            return;
        }
        if (m_invertDirection) {
            speed = -speed;
        }
        const float t = m_maxSpeed > 0.0f ? std::clamp(speed / m_maxSpeed, 0.0f, 1.0f) : 0.0f;
        const float desired = glm::mix(m_minScale, m_maxScale, t);
        const float blend = m_smoothness <= 0.0f ? 1.0f : 1.0f - std::exp(-m_smoothness * deltaTime);
        component = glm::mix(component, desired, blend);
    };

    apply(m_scaleX, localVelocity.x, scale.x);
    apply(m_scaleY, localVelocity.y, scale.y);
    apply(m_scaleZ, localVelocity.z, scale.z);

    target->SetScale(scale);
}
