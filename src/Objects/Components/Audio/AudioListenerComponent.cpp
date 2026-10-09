#include "AudioListenerComponent.hpp"

#include <glm/glm.hpp>

void AudioListenerComponent::OnUpdate(float deltaTime) {
    if (!m_enabled) return;

    (void)deltaTime;

    // Get Transform from component's owner
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) return;

    Transform* audioListenTransform = owner->GetTransform();
    if (!audioListenTransform) return;

    // Get position and forward direction from Transform
    const glm::vec3 audioListenPos = audioListenTransform->GetPosition();
    const glm::quat audioListenRot = audioListenTransform->GetRotation();
    const glm::vec3 audioListenForward = audioListenRot * glm::vec3(0.0f, 0.0f, 1.0f);

    // Set listener position and direction
    AudioManager::GetInstance().SetListenerPosition(audioListenPos);
    AudioManager::GetInstance().SetListenerDirection(audioListenForward);
}