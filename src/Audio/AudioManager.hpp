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

    enum class AudioGroupType {
        BGM = 0,
        SFX = 1
    };

    // Returns a pointer to the cached clip, or loads it if not present
    AudioClip* LoadClip(const std::string& filepath);

    // Create audio clip and assign sound ID
    uint64_t CreateClip(AudioClip* audioClip, AudioGroupType groupType = AudioGroupType::SFX);

    // Start or stop given clip
    void StartClip(uint64_t soundID);
    void StopClip(uint64_t soundID);

    // True if the sound still exists in the manager (silent check)
    bool IsValidClip(uint64_t soundID) const { return m_PlayingSounds.find(soundID) != m_PlayingSounds.end(); }

    // Releases ownership; the manager frees the sound once it finishes playing
    void DetachClip(uint64_t soundID);

    // Set group volume
    static constexpr float MinVolume = 0.0f;
    static constexpr float MaxVolume = 100.0f;

    void SetMasterGroupVolume(float volume);
    void SetAudioGroupVolume(AudioGroupType groupType, float volume);
    void ApplyVolumes(float masterVolume, float bgmVolume, float sfxVolume);

    // Set audio source position
    void SetSoundPosition(uint64_t soundID, glm::vec3 position);

    // Audio source settings
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

    static bool GetDefaultSpatialization(AudioGroupType groupType);

    // Set audio listener position and direction
    void SetListenerPosition(glm::vec3 position);
    void SetListenerDirection(glm::vec3 direction);

    // Clears the cache
    void ClearCache();

private:
    AudioManager() {
        ma_result engineResult = ma_engine_init(nullptr, &m_AudioEngine);

        if (engineResult != MA_SUCCESS) {
            std::cerr << "Failed to initialize audio engine." << std::endl;
        } else {
            m_AudioEngineInitialized = true;
            ma_result masterResult = ma_sound_group_init(&m_AudioEngine, 0, nullptr, &m_MasterGroup);

            if (masterResult != MA_SUCCESS) {
                std::cerr << "Failed to initialize master audio group." << std::endl;
            } else {
                m_MasterGroupInitialized = true;

                ma_result bgmResult = ma_sound_group_init(&m_AudioEngine, 0, &m_MasterGroup, &m_BgmGroup);
                if (bgmResult != MA_SUCCESS) {
                    std::cerr << "Failed to initialize BGM group." << std::endl;
                } else {
                    m_BgmGroupInitialized = true;
                }

                ma_result sfxResult = ma_sound_group_init(&m_AudioEngine, 0, &m_MasterGroup, &m_SfxGroup);
                if (sfxResult != MA_SUCCESS) {
                    std::cerr << "Failed to initialize SFX group." << std::endl;
                } else {
                    m_SfxGroupInitialized = true;
                }
            }
        }
    }
    ~AudioManager() { 
        ClearCache(); 

        if (m_BgmGroupInitialized) {
            ma_sound_group_uninit(&m_BgmGroup);
            m_BgmGroupInitialized = false;
        }

        if (m_SfxGroupInitialized) {
            ma_sound_group_uninit(&m_SfxGroup);
            m_SfxGroupInitialized = false;
        }

        if (m_MasterGroupInitialized) {
            ma_sound_group_uninit(&m_MasterGroup);
            m_MasterGroupInitialized = false;
        }

        if (m_AudioEngineInitialized) {
            ma_engine_uninit(&m_AudioEngine);
            m_AudioEngineInitialized = false;
        }
    }
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;  

    struct AudioSoundDeleter {
        void operator()(ma_sound* sound) const;
    };

    ma_engine m_AudioEngine;
    bool m_AudioEngineInitialized = false;

    ma_sound_group m_MasterGroup;
    bool m_MasterGroupInitialized = false;
    ma_sound_group m_BgmGroup;
    bool m_BgmGroupInitialized = false;
    ma_sound_group m_SfxGroup;
    bool m_SfxGroupInitialized = false;

    uint64_t m_NextSoundID = 1;
    std::unordered_map<uint64_t, std::unique_ptr<ma_sound, AudioSoundDeleter>> m_PlayingSounds;
    std::unordered_set<uint64_t> m_DetachedSounds;

    std::unordered_map<std::string, std::unique_ptr<AudioClip>> m_AudioCache;

    // Engine, groups, & sound checks
    bool EngineReady(std::string_view action) const;
    bool GroupsReady(std::string_view action) const;
    ma_sound* FindSound(uint64_t soundID, std::string_view action);

    ma_sound_group* GetGroup(AudioGroupType groupType);
    ma_uint32 GetSoundFlag(AudioGroupType groupType);

    // Convert the engine's left-handed space to miniaudio's right-handed space
    static glm::vec3 ConvertHandedness (const glm::vec3& v) { return glm::vec3(v.x, v.y, -v.z); }

    void ClearClips();
};