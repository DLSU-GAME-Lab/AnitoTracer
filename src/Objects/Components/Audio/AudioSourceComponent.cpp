#include "AudioSourceComponent.hpp"

void AudioSourceComponent::OnUpdate(float deltaTime) {
    if (m_soundID == 0) return;
    (void)deltaTime;

    UpdateSoundPosition();
}

void AudioSourceComponent::UpdateSoundPosition() {
    // Get Transform from component's owner
    HierarchyObject* owner = GetOwner().GetPtr();
    if (!owner) return;

    Transform* audioSourceTransform = owner->GetTransform();
    if (!audioSourceTransform) return;

    // Get position from Transform
    const glm::vec3 audioSourcePos = audioSourceTransform->GetPosition();

    // Set sound position
    AudioManager::GetInstance().SetSoundPosition(m_soundID, audioSourcePos);
}

void AudioSourceComponent::Play() {
    if (!HasAudioClip()) return;
    if (m_soundID != 0) AudioManager::GetInstance().StopClip(m_soundID);
    
    m_soundID = AudioManager::GetInstance().CreateClip(m_audioClip.Get());
    if (m_soundID == 0) return;
    
    AudioManager::GetInstance().SetSoundVolume(m_soundID, m_soundVolume);
    UpdateSoundPosition();
    AudioManager::GetInstance().StartClip(m_soundID);
}

void AudioSourceComponent::Stop() {
    if (m_soundID == 0) return;
    AudioManager::GetInstance().StopClip(m_soundID);
    m_soundID = 0;
}

void AudioSourceComponent::SetVolume(float soundVolume) {
    m_soundVolume = soundVolume;
    if (m_soundID != 0) AudioManager::GetInstance().SetSoundVolume(m_soundID, m_soundVolume);
}