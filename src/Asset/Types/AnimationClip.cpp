#include "AnimationClip.hpp"

#include "File/Parser.hpp"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <iostream>
#include <memory>
#include <fstream>
#include <unordered_map>
#include <utility>

namespace {
    std::filesystem::path PathKey(const std::filesystem::path& path) {
        auto absolute = std::filesystem::absolute(path).lexically_normal();
        auto key = absolute.generic_wstring();
#ifdef _WIN32
        std::transform(key.begin(), key.end(), key.begin(),
            [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
#endif
        return key;
    }

    std::unordered_map<std::filesystem::path, std::unique_ptr<AnimationClip>>& ClipCache() {
        static std::unordered_map<std::filesystem::path, std::unique_ptr<AnimationClip>> cache;
        return cache;
    }

    void NormalizeClip(AnimationClipData& data) {
        data.duration = std::max(0.0f, data.duration);
        data.framesPerSecond = std::max(1.0f, data.framesPerSecond);
        for (AnimationTrack& track : data.tracks) {
            const auto normalizeCurve = [](AnimationCurve& curve) {
                std::stable_sort(curve.keys.begin(), curve.keys.end(),
                    [](const AnimationKeyframe& left, const AnimationKeyframe& right) {
                        return left.time < right.time;
                    });
                for (auto key = curve.keys.begin(); key != curve.keys.end();) {
                    if (key->time < 0.0f || !std::isfinite(key->time) || !std::isfinite(key->value)) {
                        key = curve.keys.erase(key);
                    } else {
                        ++key;
                    }
                }
            };
            for (AnimationCurve& curve : track.position) normalizeCurve(curve);
            for (AnimationCurve& curve : track.rotation) normalizeCurve(curve);
            for (AnimationCurve& curve : track.scale) normalizeCurve(curve);
        }
    }
}

bool AnimationCurve::Evaluate(float time, float fallback, float& value) const {
    if (keys.empty()) {
        value = fallback;
        return false;
    }
    if (keys.size() == 1 || time <= keys.front().time) {
        value = keys.front().value;
        return true;
    }
    if (time >= keys.back().time) {
        value = keys.back().value;
        return true;
    }

    const auto upper = std::upper_bound(keys.begin(), keys.end(), time,
        [](float sampleTime, const AnimationKeyframe& key) { return sampleTime < key.time; });
    const AnimationKeyframe& right = *upper;
    const AnimationKeyframe& left = *(upper - 1);
    if (left.interpolation == AnimationInterpolation::Step || right.time <= left.time) {
        value = left.value;
        return true;
    }

    const float alpha = (time - left.time) / (right.time - left.time);
    value = left.value + (right.value - left.value) * alpha;
    return true;
}

AnimationClip::AnimationClip() {
    SetAssetType(gbe::AssetType::register_value(ASSETTYPE_ANIMATION));
}

bool AnimationClip::LoadFromFile(const std::filesystem::path& path) {
    AnimationClipData loaded;
    if (!gbe::Parser::PopulateClass(loaded, path)) {
        std::cerr << "[Animation] Failed to parse " << path << '\n';
        return false;
    }
    if (loaded.version != 1) {
        std::cerr << "[Animation] Unsupported clip version in " << path << ": "
                  << loaded.version << '\n';
        return false;
    }
    NormalizeClip(loaded);
    m_data = std::move(loaded);
    SetPath(path);
    return true;
}

bool AnimationClip::SaveToFile(const std::filesystem::path& path) const {
    std::error_code error;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) return false;
    }

    AnimationClipData normalized = m_data;
    NormalizeClip(normalized);
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) return false;
    stream << gbe::Parser::ExportClassStr(normalized) << '\n';
    return static_cast<bool>(stream);
}

AnimationClip* AnimationAssetRegistry::Load(const std::filesystem::path& path) {
    const auto key = PathKey(path);
    auto& clip = ClipCache()[key];
    if (!clip) clip = std::make_unique<AnimationClip>();
    if (!clip->LoadFromFile(path)) {
        return nullptr;
    }
    return clip.get();
}

AnimationClip* AnimationAssetRegistry::FindByPath(const std::filesystem::path& path) {
    const auto found = ClipCache().find(PathKey(path));
    return found == ClipCache().end() ? nullptr : found->second.get();
}
