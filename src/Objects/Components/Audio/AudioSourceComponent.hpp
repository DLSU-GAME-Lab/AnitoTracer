#pragma once

#include "Components/ComponentBase.hpp"
#include "AssetRef.hpp"
#include "AudioManager.hpp"
#include "Types/UpdateTrigger.hpp"
#include "HierarchyObject.hpp"
#include "AudioClip.hpp"

class AudioSourceComponent : public ComponentBase, public gbe::ITrigger<UpdateTrigger> {
public:
    // Initializes the component with an optional loaded audio clip and owner
    AudioSourceComponent (gbe::AssetRef<AudioClip> audioClip = gbe::AssetRef<AudioClip>(), 
        gbe::IInstanceManager<HierarchyObject>::Ref owner = {}) : ComponentBase("AudioSourceComponent", owner), 
        m_audioClip(audioClip) {}

    ~AudioSourceComponent() override { Stop(); }

    // Delete copy constructor/assignment to prevent object slicing and resource duplication
    AudioSourceComponent(const AudioSourceComponent&) = delete;
    AudioSourceComponent& operator=(const AudioSourceComponent&) = delete;

    // Allow moving for container compatibility
    AudioSourceComponent(AudioSourceComponent&&) = default;
    AudioSourceComponent& operator=(AudioSourceComponent&&) = default;

    void OnUpdate(float deltaTime) override;

    // Audio Source Setters / Getters

    void SetAudioClip(gbe::AssetRef<AudioClip> audioClip) { m_audioClip = audioClip; }
    gbe::AssetRef<AudioClip> GetAudioClip() const { return m_audioClip; }
    gbe::AssetRef<AudioClip>& GetAudioClipRef() { return m_audioClip; }
    bool HasAudioClip() const { return !m_audioClip.IsEmpty(); }

    void Play();
    void Stop();

    void SetVolume(float soundVolume);
    float GetVolume() const { return m_soundVolume; }

private:
    gbe::AssetRef<AudioClip> m_audioClip;
    uint64_t m_soundID = 0;

    void UpdateSoundPosition();

    GBE_SERIALIZE_FIELD(m_audioClip);

    float m_soundVolume = 100.0f;
    GBE_SERIALIZE_FIELD(m_soundVolume);

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(AudioSourceComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(AudioSourceComponent, ComponentBase);