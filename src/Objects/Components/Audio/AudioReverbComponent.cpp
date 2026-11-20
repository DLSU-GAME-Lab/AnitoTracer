#include "AudioReverbComponent.hpp"
#include <numbers>
#include <cmath>

void AudioReverbComponent::OnUpdate(float deltaTime) {
    (void)deltaTime;

    // Get Transform from component's owner
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) return;

    Transform* reverbTransform = owner->GetTransform();
    if (!reverbTransform) return;

    // Get position from Transform
    const glm::vec3 audioReverbPos = reverbTransform->GetPosition();
    const glm::vec3 audioListenPos = AudioManager::GetInstance().GetListenerPosition();

    float distance = glm::distance(audioReverbPos, audioListenPos);

    static int frameCounter = 0;
    if (++frameCounter % 120 == 1) 
        std::cout << "[AudioReverb] update pos=(" << audioReverbPos.x << "," << audioReverbPos.y << ","
            << audioReverbPos.z << ") distance=(" << distance << ")" << std::endl;

    float reverbRadius = std::clamp(m_reverbRadius, 0.0f, 1000.0f);
    float fadeDistance = std::clamp(m_fadeDistance, 0.0f, 500.0f);
    float outerEdge = reverbRadius + fadeDistance;

    float strength = 0.0f;

    if (distance <= reverbRadius) {
        strength = 1.0f;
    } else if (distance >= outerEdge) {
        strength = 0.0f;
    } else {
        strength = (outerEdge - distance) / fadeDistance;
    }

    if (strength > 0.0f) {
        if (!m_isActive) {
            AudioManager::GetInstance().SetSfxReverbRoomSize(m_reverbRoomSize);
            AudioManager::GetInstance().SetSfxReverbDamping(m_reverbDamping);
            m_isActive = true;
        }
        AudioManager::GetInstance().SetSfxReverbWet(strength * m_reverbWet, m_rampMs);
    } else if (strength == 0.0f && m_isActive) {
        AudioManager::GetInstance().SetSfxReverbWet(0.0f, m_rampMs);
        m_isActive = false;
    }
}

void AudioReverbComponent::AddCircle(std::vector<DebugLineVertex>& out, const glm::vec3& center, float radius, int axisA, int axisB, const glm::vec4& color) {
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

void AudioReverbComponent::AddSphere(std::vector<DebugLineVertex>& out, const glm::vec3& center, float radius, const glm::vec4& color) {
    AddCircle(out, center, radius, 0, 2, color); // XZ plane
    AddCircle(out, center, radius, 0, 1, color); // XY plane
    AddCircle(out, center, radius, 1, 2, color); // YZ plane
}

void AudioReverbComponent::AppendDebugLines(std::vector<DebugLineVertex>& out) const {
    // Get Transform from component's owner
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) return;

    Transform* reverbTransform = owner->GetTransform();
    if (!reverbTransform) return;

    // Get reverb center from Transform
    const glm::vec3 reverbCenter = reverbTransform->GetPosition();

    // Get inner and outer radii
    float reverbRadius = std::clamp(m_reverbRadius, 0.0f, 1000.0f);
    float fadeDistance = std::clamp(m_fadeDistance, 0.0f, 500.0f);
    float outerEdge = reverbRadius + fadeDistance;

     // Reverb area colors
    const glm::vec4 innerIdleColor(0.65f, 0.35f, 1.0f, 1.0f);
    const glm::vec4 innerActiveColor(1.0f, 0.3f, 0.8f, 1.0f);
    glm::vec4 innerColor = m_isActive ? innerActiveColor : innerIdleColor;
    const glm::vec4 outerColor(0.5f, 0.45f, 0.65f, 1.0f);

    AddSphere(out, reverbCenter, reverbRadius, innerColor);
    AddSphere(out, reverbCenter, outerEdge, outerColor);
}