#include "AudioZoneComponent.hpp"
#include <numbers>
#include <cmath>

void AudioZoneComponent::OnUpdate(float deltaTime) {
    (void)deltaTime;

    // Get Transform from component's owner
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) return;

    Transform* zoneTransform = owner->GetTransform();
    if (!zoneTransform) return;

    // Get positions from Transform
    const glm::vec3 audioZonePos = zoneTransform->GetPosition();
    const glm::vec3 audioListenPos = AudioManager::GetInstance().GetListenerPosition();

    float distance = glm::distance(audioZonePos, audioListenPos);

    // (DEBUG) Log zone position and listener distance
    /* static int frameCounter = 0;
    if (++frameCounter % 120 == 1) 
        std::cout << "[AudioZone] update pos=(" << audioZonePos.x << "," << audioZonePos.y << ","
            << audioZonePos.z << ") distance=(" << distance << ")" << std::endl; */

    // Get fade duration and zone radius settings
    const uint64_t fadeLength = GetFadeLength();
    float enterRadius = std::clamp(m_enterRadius, 0.0f, 1000.0f);
    float exitMargin = std::clamp(m_exitMargin, 0.0f, 500.0f);

    if (m_audioClip.IsEmpty()) return;

    if (!m_isInsideZone && distance < enterRadius) {
        m_activeKey = MakeOwnerKey();
        std::cout << "[AudioZone] push key=" << m_activeKey << std::endl;
        AudioManager::GetInstance().PushMusic(m_activeKey, m_audioClip.Get(), fadeLength);
        m_isInsideZone = true;
    } else if (m_isInsideZone && distance > (enterRadius + exitMargin)) {
        AudioManager::GetInstance().PopMusic(m_activeKey, fadeLength);
        m_isInsideZone = false;
    }
}

void AudioZoneComponent::AddCircle(std::vector<DebugLineVertex>& out, const glm::vec3& center, float radius, int axisA, int axisB, const glm::vec4& color) {
    constexpr int segments = 32;
    constexpr float pi = std::numbers::pi_v<float>;

    for (int i = 0; i < segments; i++) {
        // Start and end angles of each segment
        float t0 = (2 * pi * i) / segments;
        float t1 = (2 * pi * (i + 1)) / segments;

        // Starting point along circle
        glm::vec3 p0 = center;
        p0[axisA] += radius * std::cos(t0);
        p0[axisB] += radius * std::sin(t0);

        // Ending point along circle
        glm::vec3 p1 = center;
        p1[axisA] += radius * std::cos(t1);
        p1[axisB] += radius * std::sin(t1);

        // Push both points
        out.push_back({ p0, color });
        out.push_back({ p1, color });
    }
}

void AudioZoneComponent::AddSphere(std::vector<DebugLineVertex>& out, const glm::vec3& center, float radius, const glm::vec4& color) {
    AddCircle(out, center, radius, 0, 2, color); // XZ plane
    AddCircle(out, center, radius, 0, 1, color); // XY plane
    AddCircle(out, center, radius, 1, 2, color); // YZ plane
}

void AudioZoneComponent::AppendDebugLines(std::vector<DebugLineVertex>& out) const {
    // Get Transform from component's owner
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) return;

    Transform* zoneTransform = owner->GetTransform();
    if (!zoneTransform) return;

    // Get zone center from Transform
    const glm::vec3 zoneCenter = zoneTransform->GetPosition();

    // Get entry and exit radii
    float enterRadius = std::clamp(m_enterRadius, 0.0f, 1000.0f);
    float exitMargin = std::clamp(m_exitMargin, 0.0f, 500.0f);
    float exitRadius = enterRadius + exitMargin;

     // Zone colors
    const glm::vec4 enterIdleColor(1.0f, 0.75f, 0.1f, 1.0f);
    const glm::vec4 enterActiveColor(0.2f, 1.0f, 0.3f, 1.0f);
    glm::vec4 enterColor = m_isInsideZone ? enterActiveColor : enterIdleColor;
    const glm::vec4 exitColor(0.45f, 0.5f, 0.6f, 1.0f);

    AddSphere(out, zoneCenter, enterRadius, enterColor);
    AddSphere(out, zoneCenter, exitRadius, exitColor);
}