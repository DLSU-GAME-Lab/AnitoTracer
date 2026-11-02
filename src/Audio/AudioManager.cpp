#include "AudioManager.hpp"

bool AudioManager::EngineReady(std::string_view action) const {
    if (!m_AudioEngineInitialized) 
        std::cerr << action << " failed: audio engine not initialized." << std::endl;

    return m_AudioEngineInitialized;
}

ma_sound* AudioManager::FindSound(uint64_t soundID, std::string_view action) {
    if (!EngineReady(action)) return nullptr;

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

uint64_t AudioManager::CreateClip(AudioClip* audioClip) {
    if (!EngineReady("Create clip")) return 0;

    if (!audioClip) {
        std::cerr << "Audio clip not found." << std::endl;
        return 0;
    }

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

    // Get the file path from audioClip
    std::filesystem::path path = audioClip->GetPath();
    std::cout << "[Audio] CreateClip path='" << path.string() << "' exists=" << std::filesystem::exists(path)
        << " cwd='" << std::filesystem::current_path().string() << "' liveSounds=" << m_PlayingSounds.size()
        << " detached=" << m_DetachedSounds.size()
        << " engineVolume=" << ma_engine_get_volume(&m_AudioEngine)
        << " listeners=" << ma_engine_get_listener_count(&m_AudioEngine) << std::endl;

    // Alloc + init new ma_sound from that path
    ma_sound* newSound = new ma_sound;
    ma_result soundResult;

    soundResult = ma_sound_init_from_file(&m_AudioEngine, path.string().c_str(), 0, nullptr, nullptr, newSound);

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

void AudioManager::ClearCache() {
    m_AudioCache.clear();
    ClearClips();
}

void AudioManager::ClearClips() {
    std::cout << "[Audio] ClearClips: dropping " << m_PlayingSounds.size() << " sounds" << std::endl;
    m_DetachedSounds.clear();
    m_PlayingSounds.clear();
}