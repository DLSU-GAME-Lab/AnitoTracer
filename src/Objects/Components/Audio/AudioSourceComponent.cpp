#include "AudioSourceComponent.hpp"

void AudioSourceComponent::OnUpdate(float deltaTime) {
    if (m_soundID == 0) return;
    (void)deltaTime;

    // The sound may have been released by the manager; drop the stale ID instead of spamming errors
    if (!AudioManager::GetInstance().IsValidClip(m_soundID)) {
        m_soundID = 0;
        return;
    }

    UpdateSoundPosition();
}

void AudioSourceComponent::OnStart() {
    if (m_soundPlayOnAwake) Play();
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
    std::cout << "[AudioSource] Play this=" << this << " hasClip=" << HasAudioClip()
        << " clipPtr=" << (HasAudioClip() ? static_cast<void*>(m_audioClip.Get()) : nullptr)
        << " oldSoundID=" << m_soundID << " volume=" << m_soundVolume
        << " loop=" << m_soundLoop << " pitch=" << m_soundPitch << std::endl;
    if (!HasAudioClip()) return;
    if (m_soundID != 0 && AudioManager::GetInstance().IsValidClip(m_soundID)) AudioManager::GetInstance().StopClip(m_soundID);
    m_soundID = AudioManager::GetInstance().CreateClip(m_audioClip.Get());
    if (m_soundID == 0) return;
    
    AudioManager::GetInstance().SetSoundVolume(m_soundID, m_soundVolume);
    AudioManager::GetInstance().SetSoundLoop(m_soundID, m_soundLoop);
    AudioManager::GetInstance().SetSoundPitch(m_soundID, m_soundPitch);
    AudioManager::GetInstance().SetSoundSpatialized(m_soundID, m_soundSpatialized);
    AudioManager::GetInstance().SetSoundDistance(m_soundID, m_soundMinDistance, m_soundMaxDistance);
    UpdateSoundPosition();
    AudioManager::GetInstance().StartClip(m_soundID);
}

void AudioSourceComponent::PlayDetached() {
    if (m_soundLoop) return;
    Play();
    if (m_soundID != 0) AudioManager::GetInstance().DetachClip(m_soundID);
    m_soundID = 0;
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

void AudioSourceComponent::SetLoop(bool soundLoop) {
    m_soundLoop = soundLoop;
    if (m_soundID != 0) AudioManager::GetInstance().SetSoundLoop(m_soundID, m_soundLoop);
}

void AudioSourceComponent::SetPitch(float soundPitch) {
    m_soundPitch = soundPitch;
    if (m_soundID != 0) AudioManager::GetInstance().SetSoundPitch(m_soundID, m_soundPitch);
}

void AudioSourceComponent::SetSpatialized(bool soundSpatialized) {
    m_soundSpatialized = soundSpatialized;
    if (m_soundID != 0) AudioManager::GetInstance().SetSoundSpatialized(m_soundID, m_soundSpatialized);
}

void AudioSourceComponent::SetDistance(float soundMinDistance, float soundMaxDistance) {
    m_soundMinDistance = soundMinDistance;
    m_soundMaxDistance = soundMaxDistance;
    if (m_soundID != 0) AudioManager::GetInstance().SetSoundDistance(m_soundID, m_soundMinDistance, m_soundMaxDistance);
}

GBE_REGISTER_METHOD(AudioSourceComponent, Play);
GBE_REGISTER_METHOD(AudioSourceComponent, Stop);