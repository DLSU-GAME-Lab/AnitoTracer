#pragma once

#include <iostream>
#include "AudioClip.hpp"
#include "AssetLoading/AssetLoader.hpp"
#include "miniaudio.h"

#include <glm/glm.hpp>
#include <algorithm>
#include <string_view>
#include <unordered_set>

class AudioManager : public gbe::AssetLoader<AudioClip> {
public:
    static AudioManager& GetInstance() {
        static AudioManager instance;
        instance.AssignSelfAsLoader();
        return instance;
    }

    // Returns a pointer to the cached clip, or loads it if not present
    AudioClip* LoadClip(const std::string& filepath);

    // Create audio clip and assign sound ID
    uint64_t CreateClip(AudioClip* audioClip);

    // Start or stop given clip
    void StartClip(uint64_t soundID);
    void StopClip(uint64_t soundID);

    // True if the sound still exists in the manager (silent check)
    bool IsValidClip(uint64_t soundID) const { return m_PlayingSounds.find(soundID) != m_PlayingSounds.end(); }

    // Releases ownership; the manager frees the sound once it finishes playing
    void DetachClip(uint64_t soundID);

    // Set audio source position
    void SetSoundPosition(uint64_t soundID, glm::vec3 position);

    // Audio source settings
    static constexpr float MinVolume = 0.0f;
    static constexpr float MaxVolume = 100.0f;

    static constexpr float MinPitch = 0.5f;
    static constexpr float MaxPitch = 2.0f;

    static constexpr float MinDistanceLimit = 0.1f;
    static constexpr float MaxDistanceLimit = 100.0f; // Adjust based on scale of scene
    static constexpr float DistanceGap = 0.1f;

    void SetSoundVolume(uint64_t soundID, float volume);
    void SetSoundLoop(uint64_t soundID, bool loop);
    void SetSoundPitch(uint64_t soundID, float pitch);
    void SetSoundSpatialized(uint64_t soundID, bool spatialized);
    void SetSoundDistance(uint64_t soundID, float minDistance, float maxDistance);

    // Set audio listener position and direction
    void SetListenerPosition(glm::vec3 position);
    void SetListenerDirection(glm::vec3 direction);

    // Clears the cache
    void ClearCache();

private:
    AudioManager() {
        ma_result result = ma_engine_init(nullptr, &m_AudioEngine);
        if (result != MA_SUCCESS) {
            std::cerr << "Failed to initialize audio engine." << std::endl;
        } else {
            m_AudioEngineInitialized = true;
        }
    }
    ~AudioManager() { ClearCache(); }
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;  

    struct AudioSoundDeleter {
        void operator()(ma_sound* sound) const;
    };

    ma_engine m_AudioEngine;
    bool m_AudioEngineInitialized = false;
    uint64_t m_NextSoundID = 1;
    std::unordered_map<uint64_t, std::unique_ptr<ma_sound, AudioSoundDeleter>> m_PlayingSounds;
    std::unordered_set<uint64_t> m_DetachedSounds;

    std::unordered_map<std::string, std::unique_ptr<AudioClip>> m_AudioCache;

    // Engine and Sound Check
    bool EngineReady(std::string_view action) const;
    ma_sound* FindSound(uint64_t soundID, std::string_view action);

    // Convert the engine's left-handed space to miniaudio's right-handed space
    static glm::vec3 ConvertHandedness (const glm::vec3& v) { return glm::vec3(v.x, v.y, -v.z); }

    void ClearClips();
};