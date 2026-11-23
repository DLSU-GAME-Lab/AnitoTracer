#include "AudioManager.hpp"

bool AudioManager::EngineReady(std::string_view action) const {
    if (!m_AudioEngineInitialized) 
        std::cerr << action << " failed: audio engine not initialized." << std::endl;

    return m_AudioEngineInitialized;
}

bool AudioManager::GroupsReady(std::string_view action) const {
    if (!EngineReady(action)) return false;

    if (!m_MasterGroupInitialized || !m_BgmGroupInitialized || !m_SfxGroupInitialized) {
        std::cerr << action << " failed: one or more audio groups are not initialized." << std::endl;
        return false;        
    }

    return true;
}

ma_sound* AudioManager::FindSound(uint64_t soundID, std::string_view action) {
    if (!GroupsReady(action)) return nullptr;

    auto it = m_PlayingSounds.find(soundID);
    if (it == m_PlayingSounds.end()) {
        std::cerr << action << " failed: audio source not found. ID: " << soundID << std::endl;
        return nullptr;
    }

    return it->second.get();
}

AudioClip* AudioManager::LoadClip(const std::string& filepath) {
    // Check & return cache
    auto it = m_AudioCache.find(filepath);
    if (it != m_AudioCache.end()) {
        return it->second.get();
    }

    auto pAudioClip = std::make_unique<AudioClip>();

    AudioClip* rawPtr = pAudioClip.get();
    rawPtr->SetPath(filepath);

    // Optimized cache insertion
    m_AudioCache.emplace(filepath, std::move(pAudioClip));

    return rawPtr;
}

/*  Custom deleter for m_PlayingSounds; release miniaudio's internal
    resources before freeing the ma_sound itself.                       */   
void AudioManager::AudioSoundDeleter::operator()(ma_sound* sound) const {
    ma_sound_uninit(sound);
    delete sound;
}

ma_sound_group* AudioManager::GetGroup(AudioGroupType groupType) {
    switch (groupType) {
        case AudioGroupType::BGM:
            return &m_BgmGroup;
        case AudioGroupType::SFX:
            return &m_SfxGroup;
    }
    
    return nullptr;
}

ma_uint32 AudioManager::GetSoundFlag(AudioGroupType groupType) {
    switch (groupType) {
        case AudioGroupType::BGM:
            return MA_SOUND_FLAG_STREAM; // Stream BGM sound sources
        case AudioGroupType::SFX:
            return 0; // Load entire SFX sound sources
    }

    return 0;
}

bool AudioManager::GetDefaultSpatialization(AudioGroupType groupType) {
    switch (groupType) {
        case AudioGroupType::BGM:
            return false;
        case AudioGroupType::SFX:
            return true;
    }

    return true;
} 

uint64_t AudioManager::CreateClip(AudioClip* audioClip, AudioGroupType groupType) {
    if (!GroupsReady("Create clip")) return 0;

    if (!audioClip) {
        std::cerr << "Audio clip not found." << std::endl;
        return 0;
    }

    ma_sound_group* group = GetGroup(groupType);
    if (!group) {
        std::cerr << "Create clip failed: unknown audio group type." << std::endl;
        return 0;
    }

    ma_uint32 soundFlag = GetSoundFlag(groupType);

    // Free detached sounds that finished playing; owned sounds are left to their owners
    for (auto it = m_DetachedSounds.begin(); it != m_DetachedSounds.end();) {
        auto sound = m_PlayingSounds.find(*it);
        if (sound == m_PlayingSounds.end()) {
            it = m_DetachedSounds.erase(it);
        } else if (ma_sound_at_end(sound->second.get())) {
            m_PlayingSounds.erase(sound);
            it = m_DetachedSounds.erase(it);
        } else {
            ++it;
        }
    }

    RemoveFadingSounds();

    // Get the file path from audioClip
    std::filesystem::path path = audioClip->GetPath();
    std::cout << "[Audio] CreateClip path='" << path.string() << "' exists=" << std::filesystem::exists(path)
        << " cwd='" << std::filesystem::current_path().string() << "' liveSounds=" << m_PlayingSounds.size()
        << " detached=" << m_DetachedSounds.size()
        << " group=" << (groupType == AudioGroupType::BGM ? "BGM" : groupType == AudioGroupType::SFX ? "SFX" : "Unknown")
        << " flag=" << soundFlag
        << " engineVolume=" << ma_engine_get_volume(&m_AudioEngine)
        << " listeners=" << ma_engine_get_listener_count(&m_AudioEngine) << std::endl;

    // Alloc + init new ma_sound from that path
    ma_sound* newSound = new ma_sound;
    ma_result soundResult;

    soundResult = ma_sound_init_from_file(&m_AudioEngine, path.string().c_str(), soundFlag, group, nullptr, newSound);

    // Check if init succeeded
    if (soundResult != MA_SUCCESS){
        std::cerr << "Failed to initialize sound. ma_result=" << soundResult << std::endl;
        delete newSound;
        return 0;
    }

    // Register new sound in the map under a new ID
    uint64_t newID = m_NextSoundID++;
    m_PlayingSounds[newID] = std::unique_ptr<ma_sound, AudioSoundDeleter>(newSound);

    return newID;
}

void AudioManager::StartClip(uint64_t soundID) {
    ma_sound* sound = FindSound(soundID, "Start clip");
    if (!sound) return;

    // Start playback
    ma_result result = ma_sound_start(sound);
    ma_vec3f pos = ma_sound_get_position(sound);
    ma_vec3f lpos = ma_engine_listener_get_position(&m_AudioEngine, 0);
    std::cout << "[Audio] StartClip id=" << soundID << " result=" << result
        << " isPlaying=" << ma_sound_is_playing(sound)
        << " volume=" << ma_sound_get_volume(sound)
        << " spatialized=" << ma_sound_is_spatialization_enabled(sound)
        << " soundPos=(" << pos.x << "," << pos.y << "," << pos.z << ")"
        << " listenerPos=(" << lpos.x << "," << lpos.y << "," << lpos.z << ")" << std::endl;
}

void AudioManager::StopClip (uint64_t soundID) {
    ma_sound* sound = FindSound(soundID, "Stop clip");
    if (!sound) return;

    // Stop playback then remove & destroy sound
    std::cout << "[Audio] StopClip id=" << soundID << std::endl;
    ma_sound_stop(sound);
    m_PlayingSounds.erase(soundID);
}

void AudioManager::DetachClip(uint64_t soundID) {
    if (IsValidClip(soundID)) m_DetachedSounds.insert(soundID);
}

// fix this its a bit messy
void AudioManager::SwitchMusic(AudioClip* newClip, uint64_t fadeLength) {
    if (!GroupsReady("Switch music")) return;

    if (!newClip) {
        std::cerr << "Audio clip not found." << std::endl;
        return;
    }

    if (newClip == m_CurrentAudioClip && IsValidClip(m_CurrentSoundID)) return;

    uint64_t id = CreateClip(newClip, AudioManager::AudioGroupType::BGM);
    if (id == 0) return;

    SetSoundLoop(id, true);
    SetSoundSpatialized(id, GetDefaultSpatialization(AudioManager::AudioGroupType::BGM));
    SetSoundVolume(id, MaxVolume);

    ma_sound* newSound = FindSound(id, "Switch music");
    if (!newSound) return;

    ma_sound_set_fade_in_milliseconds(newSound, 0.0f, 1.0f, fadeLength);
    ma_sound_start(newSound);

    if (IsValidClip(m_CurrentSoundID)) {
        ma_sound* oldSound = FindSound(m_CurrentSoundID, "Switch music");
        ma_sound_stop_with_fade_in_milliseconds(oldSound, fadeLength);
        m_FadingOutSounds.emplace(m_CurrentSoundID, ma_engine_get_time_in_milliseconds(&m_AudioEngine) + fadeLength + 100);
    }
   
    m_CurrentAudioClip = newClip;
    m_CurrentSoundID = id;
}

void AudioManager::PushMusic(uint64_t ownerID, AudioClip* ownerClip, uint64_t fadeLength) {
    auto it = std::find_if(m_MusicStack.begin(), m_MusicStack.end(), [ownerID](const MusicEntry& m) {
        return m.ownerID == ownerID;
    });

    if (it != m_MusicStack.end()) 
        m_MusicStack.erase(it);

    if (!ownerClip) {
        std::cerr << "Audio clip not found." << std::endl;
        return;
    } else {
        m_MusicStack.push_back({ownerID, ownerClip, fadeLength});
        SwitchMusic(ownerClip, fadeLength); 
    }
}

void AudioManager::PopMusic(uint64_t ownerID, uint64_t fadeLength) {
    auto it = std::find_if(m_MusicStack.begin(), m_MusicStack.end(), [ownerID](const MusicEntry& m) {
        return m.ownerID == ownerID;
    });

    if (it != m_MusicStack.end()) {
        m_MusicStack.erase(it);

        if (!m_MusicStack.empty()) {
            SwitchMusic(m_MusicStack.back().ownerClip, m_MusicStack.back().fadeLength);
        } else {
            if (IsValidClip(m_CurrentSoundID)) {
                ma_sound* sound = FindSound(m_CurrentSoundID, "Pop music");
                if (sound) {
                    ma_sound_stop_with_fade_in_milliseconds(sound, fadeLength);
                    m_FadingOutSounds.emplace(m_CurrentSoundID, ma_engine_get_time_in_milliseconds(&m_AudioEngine) + fadeLength + 100);
                }
            }

            m_CurrentAudioClip = nullptr;
            m_CurrentSoundID = 0;
        }
    }
}

void AudioManager::SetMasterGroupVolume(float volume) {
    if (!GroupsReady("Set master volume")) return;

    ma_sound_group_set_volume(&m_MasterGroup, std::clamp(volume, MinVolume, MaxVolume) / 100);
}

void AudioManager::SetAudioGroupVolume(AudioGroupType groupType, float volume) {
    if (!GroupsReady("Set group volume")) return;

    ma_sound_group* group = GetGroup(groupType);
    if (!group) {
        std::cerr << "Set group volume failed: unknown audio group type." << std::endl;
        return;
    }

    // Set group volume; volume is expressed in percentage of 0 to 100
    ma_sound_group_set_volume(group, std::clamp(volume, MinVolume, MaxVolume) / 100);
}

void AudioManager::ApplyVolumes(float masterVolume, float bgmVolume, float sfxVolume) {
    SetMasterGroupVolume(masterVolume);
    SetAudioGroupVolume(AudioGroupType::BGM, bgmVolume);
    SetAudioGroupVolume(AudioGroupType::SFX, sfxVolume);
}

void AudioManager::UpdateSfxReverb(float deltaTime) {
    if (!m_SfxReverbInitialized || m_SfxReverbWetCurrent == m_SfxReverbWetTarget) return;

    if (m_SfxReverbWetRamp == 0) {
        m_SfxReverbWetCurrent = m_SfxReverbWetTarget;
    } else {
        float rampSeconds = m_SfxReverbWetRamp / 1000.0f;
        float step = deltaTime / rampSeconds;
        float distance = m_SfxReverbWetTarget - m_SfxReverbWetCurrent;

        if (std::abs(distance) < step) {
            m_SfxReverbWetCurrent = m_SfxReverbWetTarget;
        } else {
            if (distance > 0) {
                m_SfxReverbWetCurrent = m_SfxReverbWetCurrent + step;
            } else {
                m_SfxReverbWetCurrent = m_SfxReverbWetCurrent - step;
            }
        }
    }

    verblib_set_wet(&m_SfxReverb.reverb, m_SfxReverbWetCurrent);
}

void AudioManager::SetSfxReverbWet(float target, uint64_t rampMs) {
    if (!m_SfxReverbInitialized) return;

    m_SfxReverbWetTarget = std::clamp(target, 0.0f, 1.0f);
    m_SfxReverbWetRamp = rampMs;
}

void AudioManager::ResetSfxReverb() {
    m_SfxReverbWetCurrent = 0.0f;
    m_SfxReverbWetTarget = 0.0f;

    verblib_set_wet(&m_SfxReverb.reverb, 0.0f);
}

void AudioManager::SetSfxReverbRoomSize(float roomSize) {
    if (!m_SfxReverbInitialized) return;

    verblib_set_room_size(&m_SfxReverb.reverb, std::clamp(roomSize, 0.0f, 1.0f));
}

void AudioManager::SetSfxReverbDamping(float damping) {
    if (!m_SfxReverbInitialized) return;

    verblib_set_damping(&m_SfxReverb.reverb, std::clamp(damping, 0.0f, 1.0f));
}

void AudioManager::SetSoundPosition(uint64_t soundID, glm::vec3 position) {
    ma_sound* sound = FindSound(soundID, "Set sound position");
    if (!sound) return;

    // Set sound position
    glm::vec3 p = ConvertHandedness(position);
    ma_sound_set_position(sound, p.x, p.y, p.z);
}

void AudioManager::SetSoundVolume(uint64_t soundID, float volume) {
    ma_sound* sound = FindSound(soundID, "Set sound volume");
    if (!sound) return;

    // Set sound volume; volume is expressed in percentage of 0 to 100
    ma_sound_set_volume(sound, std::clamp(volume, MinVolume, MaxVolume) / 100);
}

void AudioManager::SetSoundLoop(uint64_t soundID, bool loop) {
    ma_sound* sound = FindSound(soundID, "Set sound loop");
    if (!sound) return;

    ma_sound_set_looping(sound, loop);
}

void AudioManager::SetSoundPitch(uint64_t soundID, float pitch) {
    ma_sound* sound = FindSound(soundID, "Set sound pitch");
    if (!sound) return;

    ma_sound_set_pitch(sound, std::clamp(pitch, MinPitch, MaxPitch));
}

void AudioManager::SetSoundSpatialized(uint64_t soundID, bool spatialized) {
    ma_sound* sound = FindSound(soundID, "Set sound spatialized");
    if (!sound) return;

    ma_sound_set_spatialization_enabled(sound, spatialized);
}

void AudioManager::SetSoundDistance(uint64_t soundID, float minDistance, float maxDistance) {
    ma_sound* sound = FindSound(soundID, "Set sound distance");
    if (!sound) return;

    float clampMin = std::clamp(minDistance, MinDistanceLimit, MaxDistanceLimit - DistanceGap);
    float clampMax = std::clamp(maxDistance, clampMin + DistanceGap, MaxDistanceLimit);

    ma_sound_set_min_distance(sound, clampMin);
    ma_sound_set_max_distance(sound, clampMax);
}

void AudioManager::SetListenerPosition(glm::vec3 position) {
    if (!EngineReady("Set listener position")) return;

    m_ListenerPosition = position;

    // Set engine position
    glm::vec3 p = ConvertHandedness(position);
    ma_engine_listener_set_position(&m_AudioEngine, 0, p.x, p.y, p.z);
}

void AudioManager::SetListenerDirection(glm::vec3 direction) {
    if (!EngineReady("Set listener direction")) return;

    // Set engine direction
    glm::vec3 d = ConvertHandedness(direction);
    ma_engine_listener_set_direction(&m_AudioEngine, 0, d.x, d.y, d.z);
}

void AudioManager::RemoveFadingSounds() {
    uint64_t currentTime = ma_engine_get_time_in_milliseconds(&m_AudioEngine);

    for (auto it = m_FadingOutSounds.begin(); it != m_FadingOutSounds.end();) {
        auto sound = m_PlayingSounds.find(it->first);
        if (sound == m_PlayingSounds.end()) {
            it = m_FadingOutSounds.erase(it);
        } else if (currentTime >= it->second) {
            m_PlayingSounds.erase(sound);
            it = m_FadingOutSounds.erase(it);
        } else {
            ++it;
        }
    }
}

void AudioManager::ClearCache() {
    m_AudioCache.clear();
    ClearClips();
}

void AudioManager::ClearClips() {
    std::cout << "[Audio] ClearClips: dropping " << m_PlayingSounds.size() << " sounds" << std::endl;

    m_FadingOutSounds.clear();
    m_CurrentSoundID = 0;
    m_CurrentAudioClip = nullptr;

    m_MusicStack.clear();

    m_DetachedSounds.clear();
    m_PlayingSounds.clear();
}