#pragma once

#include "Components/ComponentBase.hpp"
#include "AssetRef.hpp"
#include "AudioManager.hpp"
#include "Types/UpdateTrigger.hpp"
#include "HierarchyObject.hpp"
#include "AudioClip.hpp"
#include "AssignableEvent/MethodRegistry.hpp"
#include "ScriptApiRegistry.hpp"

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
    void OnStart() override;

    // Audio Source Setters / Getters
    void SetAudioClip(gbe::AssetRef<AudioClip> audioClip) { m_audioClip = audioClip; }
    gbe::AssetRef<AudioClip> GetAudioClip() const { return m_audioClip; }
    gbe::AssetRef<AudioClip>& GetAudioClipRef() { return m_audioClip; }
    bool HasAudioClip() const { return !m_audioClip.IsEmpty(); }

    // Play/Stop audio
    void Play();
    void Stop();

    // Plays the clip, then releases ownership so it keeps playing after this component is destroyed
    void PlayDetached();

    // Source Settings Setters / Getters
    void SetVolume(float soundVolume);
    float GetVolume() const { return m_soundVolume; }

    void SetLoop(bool soundLoop);
    bool GetLoop() const { return m_soundLoop; }

    void SetPitch(float soundPitch);
    float GetPitch() const { return m_soundPitch; }

    void SetSpatialized(bool soundSpatialized);
    bool GetSpatialized() const { return m_soundSpatialized; }

    void SetDistance(float soundMinDistance, float soundMaxDistance);
    float GetMinDistance() const { return m_soundMinDistance; }
    float GetMaxDistance() const { return m_soundMaxDistance; }

    void SetPlayOnAwake(bool soundPlayOnAwake) { m_soundPlayOnAwake = soundPlayOnAwake; }
    bool GetPlayOnAwake() const { return m_soundPlayOnAwake; }

private:
    gbe::AssetRef<AudioClip> m_audioClip;
    uint64_t m_soundID = 0;

    void UpdateSoundPosition();

    GBE_SERIALIZE_FIELD(m_audioClip);

    float m_soundVolume = 100.0f;
    GBE_SERIALIZE_FIELD(m_soundVolume);

    bool m_soundLoop = false;
    GBE_SERIALIZE_FIELD(m_soundLoop);

    float m_soundPitch = 1.0f;
    GBE_SERIALIZE_FIELD(m_soundPitch);

    bool m_soundSpatialized = true;
    GBE_SERIALIZE_FIELD(m_soundSpatialized);

    float m_soundMinDistance = 1.0f;
    float m_soundMaxDistance = 20.0f;
    GBE_SERIALIZE_FIELD(m_soundMinDistance);
    GBE_SERIALIZE_FIELD(m_soundMaxDistance);

    bool m_soundPlayOnAwake = false;
    GBE_SERIALIZE_FIELD(m_soundPlayOnAwake);

    GBE_GENERATE_SERIALIZER_CONSTRUCTOR(AudioSourceComponent, ComponentBase);
};

GBE_REGISTER_SERIALIZED_TYPE(AudioSourceComponent, ComponentBase);

ANITO_SCRIPT_TYPE(AudioSourceComponent, "audiosource", "anito::AudioSource", true);
ANITO_SCRIPT_PROPERTY_RO(AudioSourceComponent, "audiosource", "hasClip", HasAudioClip);
ANITO_SCRIPT_PROPERTY(AudioSourceComponent, "audiosource", "volume", GetVolume, SetVolume);
ANITO_SCRIPT_PROPERTY(AudioSourceComponent, "audiosource", "loop", GetLoop, SetLoop);
ANITO_SCRIPT_PROPERTY(AudioSourceComponent, "audiosource", "pitch", GetPitch, SetPitch);
ANITO_SCRIPT_PROPERTY(AudioSourceComponent, "audiosource", "spatialized", GetSpatialized, SetSpatialized);
ANITO_SCRIPT_METHOD(AudioSourceComponent, "audiosource", "play", Play);
ANITO_SCRIPT_METHOD(AudioSourceComponent, "audiosource", "stop", Stop);
ANITO_SCRIPT_METHOD(AudioSourceComponent, "audiosource", "playDetached", PlayDetached);
ANITO_SCRIPT_METHOD(AudioSourceComponent, "audiosource", "setDistance", SetDistance);