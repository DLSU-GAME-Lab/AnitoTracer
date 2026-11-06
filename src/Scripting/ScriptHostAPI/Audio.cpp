#include "Bindings/Audio.hpp"
#include "Common.hpp"

#include "Components/Audio/AudioSourceComponent.hpp"

namespace ScriptHost::Bindings::audio {
using detail::GetComponent;

bool HasAudioSource(anito::ObjectId id) {
    return GetComponent<AudioSourceComponent>(id) != nullptr;
}

bool HasClip(anito::ObjectId id) {
    AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id);
    return audio && audio->HasAudioClip();
}

void Play(anito::ObjectId id) {
    if (AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id)) audio->Play();
}

void Stop(anito::ObjectId id) {
    if (AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id)) audio->Stop();
}

void PlayDetached(anito::ObjectId id) {
    if (AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id)) audio->PlayDetached();
}

float GetVolume(anito::ObjectId id) {
    AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id);
    return audio ? audio->GetVolume() : 0.0f;
}

void SetVolume(anito::ObjectId id, float value) {
    if (AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id)) audio->SetVolume(value);
}

bool GetLoop(anito::ObjectId id) {
    AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id);
    return audio && audio->GetLoop();
}

void SetLoop(anito::ObjectId id, bool value) {
    if (AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id)) audio->SetLoop(value);
}

float GetPitch(anito::ObjectId id) {
    AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id);
    return audio ? audio->GetPitch() : 1.0f;
}

void SetPitch(anito::ObjectId id, float value) {
    if (AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id)) audio->SetPitch(value);
}

bool GetSpatialized(anito::ObjectId id) {
    AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id);
    return audio && audio->GetSpatialized();
}

void SetSpatialized(anito::ObjectId id, bool value) {
    if (AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id)) audio->SetSpatialized(value);
}

void SetDistance(anito::ObjectId id, float minDistance, float maxDistance) {
    if (AudioSourceComponent* audio = GetComponent<AudioSourceComponent>(id)) audio->SetDistance(minDistance, maxDistance);
}

anito::AudioAPI GetAPI() {
    return {HasAudioSource, HasClip, Play, Stop, PlayDetached, GetVolume, SetVolume, GetLoop, SetLoop,
            GetPitch, SetPitch, GetSpatialized, SetSpatialized, SetDistance};
}
} // namespace ScriptHost::Bindings::audio
