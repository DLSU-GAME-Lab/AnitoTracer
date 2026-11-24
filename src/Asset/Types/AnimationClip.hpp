#pragma once

#include "IAsset.hpp"
#include "Meta/IAsset_meta.hpp"
#include "TypeConstants.hpp"

#include <array>
#include <filesystem>
#include <string>
#include <vector>

enum class AnimationInterpolation {
    Linear,
    Step
};

struct AnimationKeyframe {
    float time = 0.0f;
    float value = 0.0f;
    AnimationInterpolation interpolation = AnimationInterpolation::Linear;
};

struct AnimationCurve {
    std::vector<AnimationKeyframe> keys;

    bool Evaluate(float time, float fallback, float& value) const;
};

struct AnimationTrack {
    std::string relativePath;
    std::string objectGuid;
    std::array<AnimationCurve, 3> position;
    std::array<AnimationCurve, 3> rotation;
    std::array<AnimationCurve, 3> scale;
};

struct AnimationClipData {
    int version = 1;
    std::string name;
    float duration = 1.0f;
    float framesPerSecond = 30.0f;
    bool loop = false;
    std::vector<AnimationTrack> tracks;
};

class AnimationClip : public gbe::IAsset {
public:
    AnimationClip();

    AnimationClipData& GetData() { return m_data; }
    const AnimationClipData& GetData() const { return m_data; }

    float GetDuration() const { return m_data.duration; }
    bool IsLooping() const { return m_data.loop; }
    bool LoadFromFile(const std::filesystem::path& path);
    bool SaveToFile(const std::filesystem::path& path) const;

private:
    AnimationClipData m_data;
};

class AnimationAssetRegistry {
public:
    static AnimationClip* Load(const std::filesystem::path& path);
    static AnimationClip* FindByPath(const std::filesystem::path& path);
};

namespace glz {
    template <>
    struct meta<AnimationKeyframe> {
        using T = AnimationKeyframe;
        static constexpr auto value = object(
            "time", &T::time,
            "value", &T::value,
            "interpolation", &T::interpolation
        );
    };

    template <>
    struct meta<AnimationCurve> {
        using T = AnimationCurve;
        static constexpr auto value = object("keys", &T::keys);
    };

    template <>
    struct meta<AnimationTrack> {
        using T = AnimationTrack;
        static constexpr auto value = object(
            "relative_path", &T::relativePath,
            "object_guid", &T::objectGuid,
            "position", &T::position,
            "rotation", &T::rotation,
            "scale", &T::scale
        );
    };

    template <>
    struct meta<AnimationClipData> {
        using T = AnimationClipData;
        static constexpr auto value = object(
            "version", &T::version,
            "name", &T::name,
            "duration", &T::duration,
            "frames_per_second", &T::framesPerSecond,
            "loop", &T::loop,
            "tracks", &T::tracks
        );
    };

    template <>
    struct meta<AnimationClip> {
        static constexpr auto value = meta<gbe::IAsset>::value;
    };
}
